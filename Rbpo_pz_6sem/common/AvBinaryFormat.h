#pragma once

#include "../AvEngine.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace avstore
{
	inline constexpr char kMagic[4] = { 'R', 'B', 'P', 'O' };
	inline constexpr std::uint32_t kFormatVersion = 1;

#pragma pack(push, 1)
	struct FileHeader
	{
		char magic[4]{};
		std::uint32_t version = 0;
		std::uint32_t crc32 = 0;
		std::uint64_t payloadSize = 0;
	};
#pragma pack(pop)

	inline std::uint32_t Crc32(const BYTE* data, std::size_t size)
	{
		std::uint32_t crc = 0xFFFFFFFFu;
		for (std::size_t i = 0; i < size; ++i)
		{
			crc ^= data[i];
			for (int bit = 0; bit < 8; ++bit)
			{
				const std::uint32_t mask = static_cast<std::uint32_t>(-(crc & 1u));
				crc = (crc >> 1) ^ (0xEDB88320u & mask);
			}
		}
		return ~crc;
	}

	inline void AppendBytes(std::vector<BYTE>& buffer, const void* data, std::size_t size)
	{
		const auto* bytes = static_cast<const BYTE*>(data);
		buffer.insert(buffer.end(), bytes, bytes + size);
	}

	inline std::vector<BYTE> BuildRecordCanonicalBytes(const av::AvRecord& record)
	{
		std::vector<BYTE> data;
		data.reserve(64 + record.objectSignature.size());
		AppendBytes(data, &record.objectSignaturePrefix, sizeof(record.objectSignaturePrefix));
		AppendBytes(data, &record.objectSignatureLength, sizeof(record.objectSignatureLength));
		AppendBytes(data, record.objectSignature.data(), record.objectSignature.size());
		AppendBytes(data, &record.offsetBegin, sizeof(record.offsetBegin));
		AppendBytes(data, &record.offsetEnd, sizeof(record.offsetEnd));
		AppendBytes(data, &record.objectType, sizeof(record.objectType));
		return data;
	}

	inline std::vector<BYTE> BuildPayload(const av::AvDatabase& database)
	{
		std::vector<BYTE> payload;
		payload.reserve(16);
		AppendBytes(payload, &database.releaseDate, sizeof(database.releaseDate));

		std::uint32_t recordCount = 0;
		for (const auto& [_, records] : database.records)
		{
			recordCount += static_cast<std::uint32_t>(records.size());
		}
		AppendBytes(payload, &recordCount, sizeof(recordCount));

		for (const auto& [prefix, records] : database.records)
		{
			for (const auto& record : records)
			{
				const std::uint64_t storedPrefix = record.objectSignaturePrefix != 0 ? record.objectSignaturePrefix : prefix;
				AppendBytes(payload, &storedPrefix, sizeof(storedPrefix));
				AppendBytes(payload, &record.objectSignatureLength, sizeof(record.objectSignatureLength));
				const std::uint32_t sigLen = static_cast<std::uint32_t>(record.objectSignature.size());
				AppendBytes(payload, &sigLen, sizeof(sigLen));
				AppendBytes(payload, record.objectSignature.data(), record.objectSignature.size());
				AppendBytes(payload, &record.offsetBegin, sizeof(record.offsetBegin));
				AppendBytes(payload, &record.offsetEnd, sizeof(record.offsetEnd));
				AppendBytes(payload, &record.objectType, sizeof(record.objectType));
				const std::uint32_t recordSigLen = static_cast<std::uint32_t>(record.avRecordSignature.size());
				AppendBytes(payload, &recordSigLen, sizeof(recordSigLen));
				AppendBytes(payload, record.avRecordSignature.data(), record.avRecordSignature.size());
			}
		}

		return payload;
	}

	inline bool WriteDatabaseFile(const std::filesystem::path& path, const av::AvDatabase& database, const std::vector<BYTE>& manifestSignature)
	{
		const std::vector<BYTE> payload = BuildPayload(database);
		FileHeader header{};
		memcpy(header.magic, kMagic, sizeof(kMagic));
		header.version = kFormatVersion;
		header.payloadSize = payload.size();
		header.crc32 = Crc32(payload.data(), payload.size());

		std::vector<BYTE> fileData;
		fileData.reserve(sizeof(FileHeader) + payload.size() + sizeof(std::uint32_t) + manifestSignature.size());
		AppendBytes(fileData, &header, sizeof(header));
		AppendBytes(fileData, payload.data(), payload.size());
		const std::uint32_t manifestSigLen = static_cast<std::uint32_t>(manifestSignature.size());
		AppendBytes(fileData, &manifestSigLen, sizeof(manifestSigLen));
		AppendBytes(fileData, manifestSignature.data(), manifestSignature.size());

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		if (!output)
		{
			return false;
		}

		output.write(reinterpret_cast<const char*>(fileData.data()), static_cast<std::streamsize>(fileData.size()));
		return static_cast<bool>(output);
	}

	struct LoadedFile
	{
		std::vector<BYTE> payload;
		std::vector<BYTE> manifestSignature;
		FileHeader header{};
	};

	inline bool ReadDatabaseFile(const std::filesystem::path& path, LoadedFile& loaded)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			return false;
		}

		input.read(reinterpret_cast<char*>(&loaded.header), sizeof(FileHeader));
		if (!input || memcmp(loaded.header.magic, kMagic, sizeof(kMagic)) != 0 || loaded.header.version != kFormatVersion)
		{
			return false;
		}

		loaded.payload.resize(static_cast<std::size_t>(loaded.header.payloadSize));
		input.read(reinterpret_cast<char*>(loaded.payload.data()), static_cast<std::streamsize>(loaded.payload.size()));
		if (!input)
		{
			return false;
		}

		if (Crc32(loaded.payload.data(), loaded.payload.size()) != loaded.header.crc32)
		{
			return false;
		}

		std::uint32_t manifestSigLen = 0;
		input.read(reinterpret_cast<char*>(&manifestSigLen), sizeof(manifestSigLen));
		if (!input)
		{
			return false;
		}

		loaded.manifestSignature.resize(manifestSigLen);
		if (manifestSigLen > 0)
		{
			input.read(reinterpret_cast<char*>(loaded.manifestSignature.data()), static_cast<std::streamsize>(manifestSigLen));
			if (!input)
			{
				return false;
			}
		}

		return true;
	}

	inline bool ParsePayload(
		const std::vector<BYTE>& payload,
		av::AvDatabase& database,
		std::vector<std::vector<BYTE>>& recordCanonicalBytes)
	{
		if (payload.size() < sizeof(FILETIME) + sizeof(std::uint32_t))
		{
			return false;
		}

		std::size_t offset = 0;
		auto read = [&](void* dst, std::size_t size) -> bool
		{
			if (offset + size > payload.size())
			{
				return false;
			}
			memcpy(dst, payload.data() + offset, size);
			offset += size;
			return true;
		};

		if (!read(&database.releaseDate, sizeof(database.releaseDate)))
		{
			return false;
		}

		std::uint32_t recordCount = 0;
		if (!read(&recordCount, sizeof(recordCount)))
		{
			return false;
		}

		database.records.clear();
		recordCanonicalBytes.clear();
		recordCanonicalBytes.reserve(recordCount);

		for (std::uint32_t i = 0; i < recordCount; ++i)
		{
			av::AvRecord record{};
			std::uint64_t prefix = 0;
			if (!read(&prefix, sizeof(prefix)) ||
				!read(&record.objectSignatureLength, sizeof(record.objectSignatureLength)))
			{
				return false;
			}

			std::uint32_t sigLen = 0;
			if (!read(&sigLen, sizeof(sigLen)))
			{
				return false;
			}

			record.objectSignature.resize(sigLen);
			if (sigLen > 0 && !read(record.objectSignature.data(), sigLen))
			{
				return false;
			}

			if (!read(&record.offsetBegin, sizeof(record.offsetBegin)) ||
				!read(&record.offsetEnd, sizeof(record.offsetEnd)) ||
				!read(&record.objectType, sizeof(record.objectType)))
			{
				return false;
			}

			std::uint32_t recordSigLen = 0;
			if (!read(&recordSigLen, sizeof(recordSigLen)))
			{
				return false;
			}

			record.avRecordSignature.resize(recordSigLen);
			if (recordSigLen > 0 && !read(record.avRecordSignature.data(), recordSigLen))
			{
				return false;
			}

			record.objectSignaturePrefix = prefix;
			recordCanonicalBytes.push_back(BuildRecordCanonicalBytes(record));
			database.records[prefix].push_back(std::move(record));
		}

		return offset == payload.size();
	}
}
