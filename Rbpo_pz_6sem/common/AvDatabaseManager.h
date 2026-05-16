#pragma once

#include "AvBinaryFormat.h"
#include "AvSignature.h"

#include <functional>
#include <mutex>
#include <string>

namespace avstore
{
	enum class LoadStatus
	{
		Ok,
		FileMissing,
		IntegrityFailed,
		ManifestSignatureFailed,
		ParseFailed,
		PartialRecordsLoaded
	};

	struct LoadReport
	{
		LoadStatus status = LoadStatus::FileMissing;
		std::size_t recordsLoaded = 0;
		std::size_t recordsSkipped = 0;
	};

	using RecordFetchFn = std::function<bool(const av::AvRecord&, const std::vector<BYTE>& canonical, av::AvRecord& repaired)>;

	inline bool CopyDatabaseFile(const std::filesystem::path& from, const std::filesystem::path& to)
	{
		std::error_code ec;
		std::filesystem::create_directories(to.parent_path(), ec);
		std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
		return !ec;
	}

	inline bool InstallBundledDefaultDatabase()
	{
		const auto bundled = GetBundledDefaultDatabasePath();
		if (!std::filesystem::exists(bundled))
		{
			return false;
		}

		return CopyDatabaseFile(bundled, GetCurrentDatabasePath());
	}

	inline LoadReport LoadDatabaseFromFile(
		const std::filesystem::path& path,
		av::AvDatabase& database,
		SignatureVerifier& verifier,
		bool verifyManifest,
		bool verifyRecords,
		bool networkAvailable,
		RecordFetchFn fetchRecord)
	{
		LoadReport report{};
		LoadedFile loaded{};
		if (!ReadDatabaseFile(path, loaded))
		{
			report.status = std::filesystem::exists(path) ? LoadStatus::IntegrityFailed : LoadStatus::FileMissing;
			return report;
		}

		if (verifyManifest && !verifier.Verify(loaded.payload, loaded.manifestSignature))
		{
			report.status = LoadStatus::ManifestSignatureFailed;
			return report;
		}

		std::vector<std::vector<BYTE>> canonicalRecords;
		if (!ParsePayload(loaded.payload, database, canonicalRecords))
		{
			report.status = LoadStatus::ParseFailed;
			return report;
		}

		if (!verifyRecords)
		{
			report.status = LoadStatus::Ok;
			report.recordsLoaded = canonicalRecords.size();
			return report;
		}

		av::AvDatabase filtered{};
		filtered.releaseDate = database.releaseDate;
		report.recordsSkipped = 0;
		report.recordsLoaded = 0;

		std::size_t index = 0;
		for (auto& [prefix, records] : database.records)
		{
			std::vector<av::AvRecord> validRecords;
			for (auto& record : records)
			{
				const std::vector<BYTE>& canonical = canonicalRecords[index++];
				if (verifier.Verify(canonical, record.avRecordSignature))
				{
					validRecords.push_back(std::move(record));
					++report.recordsLoaded;
					continue;
				}

				if (networkAvailable && fetchRecord)
				{
					av::AvRecord repaired{};
					if (fetchRecord(record, canonical, repaired) &&
						verifier.Verify(BuildRecordCanonicalBytes(repaired), repaired.avRecordSignature))
					{
						validRecords.push_back(std::move(repaired));
						++report.recordsLoaded;
						continue;
					}
				}

				++report.recordsSkipped;
			}

			if (!validRecords.empty())
			{
				filtered.records[prefix] = std::move(validRecords);
			}
		}

		database = std::move(filtered);
		report.status = report.recordsSkipped > 0 ? LoadStatus::PartialRecordsLoaded : LoadStatus::Ok;
		return report;
	}

	class AvDatabaseManager
	{
	public:
		using LogFn = std::function<void(const std::wstring&)>;

		explicit AvDatabaseManager(LogFn logFn = {})
			: m_log(std::move(logFn))
		{
		}

		void SetRecordFetcher(RecordFetchFn fetcher)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_fetchRecord = std::move(fetcher);
		}

		void SetNetworkAvailable(bool available)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_networkAvailable = available;
		}

		bool InitializeVerifier()
		{
			return m_verifier.Initialize();
		}

		bool IsLoaded() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_loaded;
		}

		av::AvDatabase Snapshot() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_database;
		}

		std::size_t RecordCount() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			std::size_t total = 0;
			for (const auto& [_, records] : m_database.records)
			{
				total += records.size();
			}
			return total;
		}

		bool BackupCurrent()
		{
			const auto current = GetCurrentDatabasePath();
			if (!std::filesystem::exists(current))
			{
				return false;
			}

			return CopyDatabaseFile(current, GetBackupDatabasePath());
		}

		bool RestoreBackup()
		{
			const auto backup = GetBackupDatabasePath();
			if (!std::filesystem::exists(backup))
			{
				return false;
			}

			return CopyDatabaseFile(backup, GetCurrentDatabasePath());
		}

		bool SaveDownloadedBytes(const std::vector<BYTE>& bytes)
		{
			const auto path = GetCurrentDatabasePath();
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);

			std::ofstream output(path, std::ios::binary | std::ios::trunc);
			if (!output)
			{
				return false;
			}

			output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
			return static_cast<bool>(output);
		}

		LoadReport LoadStartupDatabase()
		{
			InitializeVerifier();

			RecordFetchFn fetcher;
			bool networkAvailable = false;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				fetcher = m_fetchRecord;
				networkAvailable = m_networkAvailable;
			}

			const auto tryLoadCurrent = [&]() -> LoadReport
			{
				av::AvDatabase database{};
				LoadReport report = LoadDatabaseFromFile(
					GetCurrentDatabasePath(),
					database,
					m_verifier,
					true,
					true,
					networkAvailable,
					fetcher);
				if (report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded)
				{
					CommitDatabase(std::move(database));
				}
				return report;
			};

			LoadReport report = tryLoadCurrent();
			if (report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded)
			{
				Log(L"AV: loaded current database, records=" + std::to_wstring(report.recordsLoaded));
				return report;
			}

			if (report.status == LoadStatus::ManifestSignatureFailed)
			{
				m_lastManifestFailure = true;
				Log(L"AV: manifest signature invalid, restoring backup");
				if (RestoreBackup())
				{
					report = tryLoadCurrent();
					if (report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded)
					{
						return report;
					}
				}
			}

			Log(L"AV: installing bundled default database");
			if (!InstallBundledDefaultDatabase())
			{
				av::AvDatabase defaults = av::CreateDefaultDatabase();
				CommitDatabase(std::move(defaults));
				Log(L"AV: using in-memory fallback defaults");
				return LoadReport{ LoadStatus::PartialRecordsLoaded, 2, 0 };
			}

			report = tryLoadCurrent();
			if (report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded)
			{
				return report;
			}

			av::AvDatabase defaults = av::CreateDefaultDatabase();
			CommitDatabase(std::move(defaults));
			Log(L"AV: using in-memory fallback defaults after install failure");
			return LoadReport{ LoadStatus::PartialRecordsLoaded, 2, 0 };
		}

		LoadReport ReloadCurrentFromDisk()
		{
			RecordFetchFn fetcher;
			bool networkAvailable = false;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				fetcher = m_fetchRecord;
				networkAvailable = m_networkAvailable;
			}

			av::AvDatabase database{};
			LoadReport report = LoadDatabaseFromFile(
				GetCurrentDatabasePath(),
				database,
				m_verifier,
				true,
				true,
				networkAvailable,
				fetcher);
			if (report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded)
			{
				CommitDatabase(std::move(database));
			}
			return report;
		}

		bool RollbackToBackup()
		{
			if (!RestoreBackup())
			{
				return false;
			}

			const LoadReport report = ReloadCurrentFromDisk();
			return report.status == LoadStatus::Ok || report.status == LoadStatus::PartialRecordsLoaded;
		}

		bool NeedsForcedUpdate() const
		{
			return m_lastManifestFailure;
		}

		void SetManifestFailurePending(bool value)
		{
			m_lastManifestFailure = value;
		}

	private:
		void CommitDatabase(av::AvDatabase database)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_database = std::move(database);
			m_loaded = true;
			m_lastManifestFailure = false;
		}

		void Log(const std::wstring& message) const
		{
			if (m_log)
			{
				m_log(message);
			}
		}

		mutable std::mutex m_mutex;
		av::AvDatabase m_database{};
		SignatureVerifier m_verifier;
		RecordFetchFn m_fetchRecord;
		bool m_loaded = false;
		bool m_lastManifestFailure = false;
		bool m_networkAvailable = false;
		LogFn m_log;
	};
}
