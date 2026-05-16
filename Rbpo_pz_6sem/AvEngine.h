#pragma once

#include <windows.h>
#include <wincrypt.h>

#include <array>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace av
{
	enum class ObjectType : std::uint8_t
	{
		Pe = 1,
		PythonScript = 2
	};

	struct AvRecord
	{
		std::uint64_t objectSignaturePrefix = 0;
		std::uint32_t objectSignatureLength = 0;
		std::vector<BYTE> objectSignature;
		std::int64_t offsetBegin = 0;
		std::int64_t offsetEnd = 0;
		ObjectType objectType = ObjectType::Pe;
		std::vector<BYTE> avRecordSignature;
	};

	struct AvDatabase
	{
		FILETIME releaseDate{};
		std::map<std::uint64_t, std::vector<AvRecord>> records;
	};

	struct ScanFinding
	{
		bool infected = false;
		std::wstring matchedPath;
		std::wstring matchedSignature;
		std::uint64_t offset = 0;
		ObjectType objectType = ObjectType::Pe;
	};

	class IByteStream
	{
	public:
		virtual ~IByteStream() = default;
		virtual bool Seek(std::uint64_t position) = 0;
		virtual std::uint64_t Position() const = 0;
		virtual std::uint64_t Size() const = 0;
		virtual bool ReadExact(void* buffer, std::size_t bytes) = 0;
	};

	inline std::uint64_t Fnv1a64(const BYTE* data, std::size_t size)
	{
		constexpr std::uint64_t offsetBasis = 1469598103934665603ULL;
		constexpr std::uint64_t prime = 1099511628211ULL;
		std::uint64_t hash = offsetBasis;
		for (std::size_t i = 0; i < size; ++i)
		{
			hash ^= data[i];
			hash *= prime;
		}
		return hash;
	}

	inline std::vector<BYTE> ToBytes(std::uint64_t value)
	{
		std::vector<BYTE> bytes(sizeof(value));
		for (std::size_t i = 0; i < sizeof(value); ++i)
		{
			bytes[i] = static_cast<BYTE>((value >> (i * 8)) & 0xFF);
		}
		return bytes;
	}

	inline std::uint64_t ReadU64(const BYTE* bytes)
	{
		std::uint64_t value = 0;
		for (std::size_t i = 0; i < 8; ++i)
		{
			value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
		}
		return value;
	}

	inline std::vector<BYTE> BuildRecordSignature(const AvRecord& record)
	{
		std::vector<BYTE> data;
		data.reserve(sizeof(record.objectSignaturePrefix) + sizeof(record.objectSignatureLength) + record.objectSignature.size() + sizeof(record.offsetBegin) + sizeof(record.offsetEnd) + sizeof(record.objectType));

		auto append = [&data](const void* ptr, std::size_t size)
		{
			const auto* bytes = static_cast<const BYTE*>(ptr);
			data.insert(data.end(), bytes, bytes + size);
		};

		append(&record.objectSignaturePrefix, sizeof(record.objectSignaturePrefix));
		append(&record.objectSignatureLength, sizeof(record.objectSignatureLength));
		append(record.objectSignature.data(), record.objectSignature.size());
		append(&record.offsetBegin, sizeof(record.offsetBegin));
		append(&record.offsetEnd, sizeof(record.offsetEnd));
		append(&record.objectType, sizeof(record.objectType));

		const std::uint64_t hash = Fnv1a64(data.data(), data.size());
		return ToBytes(hash);
	}

	inline AvRecord MakeRecord(std::uint64_t prefix, ObjectType type, std::int64_t offsetBegin, std::int64_t offsetEnd, const std::vector<BYTE>& tailBytes)
	{
		AvRecord record{};
		record.objectSignaturePrefix = prefix;
		record.objectSignatureLength = static_cast<std::uint32_t>(sizeof(prefix) + tailBytes.size());
		record.objectSignature = ToBytes(Fnv1a64(tailBytes.data(), tailBytes.size()));
		record.offsetBegin = offsetBegin;
		record.offsetEnd = offsetEnd;
		record.objectType = type;
		record.avRecordSignature = BuildRecordSignature(record);
		return record;
	}

	inline AvDatabase CreateDefaultDatabase()
	{
		AvDatabase db{};
		GetSystemTimeAsFileTime(&db.releaseDate);

		const std::uint64_t pePrefix = 0x0000000000005A4DULL; // "MZ"
		const std::uint64_t pyPrefix = 0x2020202020202321ULL; // "!#      "

		auto peTail = std::vector<BYTE>{'P', 'E', 0, 0, 'M', 'A', 'L', '1'};
		auto pyTail = std::vector<BYTE>{'p', 'y', 't', 'h', 'o', 'n', 0, 1};

		db.records[pePrefix].push_back(MakeRecord(pePrefix, ObjectType::Pe, 0, 4096, peTail));
		db.records[pyPrefix].push_back(MakeRecord(pyPrefix, ObjectType::PythonScript, 0, 4096, pyTail));
		return db;
	}

	inline std::wstring FormatReleaseDate(const FILETIME& ft)
	{
		SYSTEMTIME utc{};
		SYSTEMTIME local{};
		FileTimeToSystemTime(&ft, &utc);
		SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local);
		wchar_t buffer[64]{};
		swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u", local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute, local.wSecond);
		return buffer;
	}

	inline std::wstring ObjectTypeName(ObjectType type)
	{
		switch (type)
		{
		case ObjectType::Pe:
			return L"PE";
		case ObjectType::PythonScript:
			return L"Python";
		default:
			return L"Unknown";
		}
	}

	class FileByteStream final : public IByteStream
	{
	public:
		explicit FileByteStream(const std::wstring& path)
			: m_path(path)
		{
		}

		~FileByteStream() override
		{
			Close();
		}

		bool Open()
		{
			Close();
			m_handle = CreateFileW(m_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (m_handle == INVALID_HANDLE_VALUE)
			{
				m_handle = nullptr;
				return false;
			}

			LARGE_INTEGER li{};
			if (!GetFileSizeEx(m_handle, &li))
			{
				Close();
				return false;
			}

			m_size = static_cast<std::uint64_t>(li.QuadPart);
			m_position = 0;
			return true;
		}

		bool Seek(std::uint64_t position) override
		{
			if (!m_handle)
			{
				return false;
			}

			LARGE_INTEGER li{};
			li.QuadPart = static_cast<LONGLONG>(position);
			if (!SetFilePointerEx(m_handle, li, nullptr, FILE_BEGIN))
			{
				return false;
			}

			m_position = position;
			return true;
		}

		std::uint64_t Position() const override
		{
			return m_position;
		}

		std::uint64_t Size() const override
		{
			return m_size;
		}

		bool ReadExact(void* buffer, std::size_t bytes) override
		{
			if (!m_handle)
			{
				return false;
			}

			DWORD read = 0;
			if (!ReadFile(m_handle, buffer, static_cast<DWORD>(bytes), &read, nullptr) || read != bytes)
			{
				return false;
			}

			m_position += bytes;
			return true;
		}

	private:
		void Close()
		{
			if (m_handle)
			{
				CloseHandle(m_handle);
				m_handle = nullptr;
			}
		}

		std::wstring m_path;
		HANDLE m_handle = nullptr;
		std::uint64_t m_size = 0;
		std::uint64_t m_position = 0;
	};

	inline bool DetectObjectType(const std::wstring& path, ObjectType& type)
	{
		const auto extPos = path.find_last_of(L'.');
		std::wstring ext = (extPos == std::wstring::npos) ? L"" : path.substr(extPos);
		for (auto& ch : ext)
		{
			ch = static_cast<wchar_t>(towlower(ch));
		}

		if (ext == L".exe" || ext == L".dll" || ext == L".sys")
		{
			type = ObjectType::Pe;
			return true;
		}

		if (ext == L".py")
		{
			type = ObjectType::PythonScript;
			return true;
		}

		type = ObjectType::Pe;
		return true;
	}

	inline std::wstring ReadableBytesToHex(const BYTE* data, std::size_t size)
	{
		std::wstring result;
		result.reserve(size * 2);
		const wchar_t* hex = L"0123456789ABCDEF";
		for (std::size_t i = 0; i < size; ++i)
		{
			result.push_back(hex[(data[i] >> 4) & 0xF]);
			result.push_back(hex[data[i] & 0xF]);
		}
		return result;
	}

	struct PrefixAhoNode
	{
		std::map<BYTE, std::size_t> next;
		std::size_t fail = 0;
		std::vector<std::uint64_t> outputs;
	};

	inline std::vector<PrefixAhoNode> BuildPrefixAutomaton(const AvDatabase& db)
	{
		std::vector<PrefixAhoNode> nodes(1);
		for (const auto& [key, records] : db.records)
		{
			std::array<BYTE, 8> bytes{};
			for (std::size_t i = 0; i < bytes.size(); ++i)
			{
				bytes[i] = static_cast<BYTE>((key >> (i * 8)) & 0xFF);
			}

			std::size_t state = 0;
			for (BYTE b : bytes)
			{
				auto it = nodes[state].next.find(b);
				if (it == nodes[state].next.end())
				{
					nodes[state].next[b] = nodes.size();
					nodes.push_back({});
					state = nodes.size() - 1;
				}
				else
				{
					state = it->second;
				}
			}

			nodes[state].outputs.push_back(key);
		}

		std::vector<std::size_t> queue;
		for (const auto& [byteValue, child] : nodes[0].next)
		{
			nodes[child].fail = 0;
			queue.push_back(child);
		}

		for (std::size_t head = 0; head < queue.size(); ++head)
		{
			const std::size_t state = queue[head];
			for (const auto& [byteValue, nextState] : nodes[state].next)
			{
				std::size_t failState = nodes[state].fail;
				while (failState != 0 && nodes[failState].next.find(byteValue) == nodes[failState].next.end())
				{
					failState = nodes[failState].fail;
				}

				auto failIt = nodes[failState].next.find(byteValue);
				if (failIt != nodes[failState].next.end())
				{
					nodes[nextState].fail = failIt->second;
				}
				else
				{
					nodes[nextState].fail = 0;
				}

				const auto& failOutputs = nodes[nodes[nextState].fail].outputs;
				nodes[nextState].outputs.insert(nodes[nextState].outputs.end(), failOutputs.begin(), failOutputs.end());
				queue.push_back(nextState);
			}
		}

		return nodes;
	}

	inline bool VerifyAvRecordSignature(const AvRecord& record)
	{
		const auto expected = BuildRecordSignature(record);
		return record.avRecordSignature == expected;
	}

	inline bool MatchRecord(IByteStream& stream, const AvRecord& record, std::uint64_t scanStart, const std::array<BYTE, 8>& prefixBytes, ScanFinding& finding)
	{
		if (!VerifyAvRecordSignature(record))
		{
			return false;
		}

		const std::size_t tailSize = record.objectSignatureLength > 8 ? record.objectSignatureLength - 8 : 0;
		std::vector<BYTE> tail(tailSize);
		const std::uint64_t savedPosition = stream.Position();
		if (tailSize > 0 && !stream.ReadExact(tail.data(), tail.size()))
		{
			return false;
		}

		std::vector<BYTE> hashInput(prefixBytes.begin(), prefixBytes.end());
		hashInput.insert(hashInput.end(), tail.begin(), tail.end());
		const std::uint64_t hash = Fnv1a64(hashInput.data(), hashInput.size());
		if (record.objectSignature != ToBytes(hash))
		{
			if (!stream.Seek(savedPosition))
			{
				return false;
			}
			return false;
		}

		finding.infected = true;
		finding.matchedSignature = ReadableBytesToHex(record.objectSignature.data(), record.objectSignature.size());
		finding.offset = scanStart;
		finding.objectType = record.objectType;
		return true;
	}

	inline bool ScanByteStreamLinear(IByteStream& stream, ObjectType objectType, const AvDatabase& db, ScanFinding& finding)
	{
		finding = {};
		if (stream.Size() < 8 || !stream.Seek(0))
		{
			return false;
		}

		std::uint64_t position = 0;
		while (position + 8 <= stream.Size())
		{
			if (!stream.Seek(position))
			{
				return false;
			}

			std::array<BYTE, 8> prefixBytes{};
			if (!stream.ReadExact(prefixBytes.data(), prefixBytes.size()))
			{
				return false;
			}

			const std::uint64_t key = ReadU64(prefixBytes.data());
			auto recordIt = db.records.find(key);
			if (recordIt != db.records.end())
			{
				std::vector<const AvRecord*> candidates;
				candidates.reserve(recordIt->second.size());
				for (const auto& record : recordIt->second)
				{
					if (record.objectType != objectType)
					{
						continue;
					}

					if (static_cast<std::int64_t>(position) < record.offsetBegin || static_cast<std::int64_t>(position) > record.offsetEnd)
					{
						continue;
					}

					candidates.push_back(&record);
				}

				for (const AvRecord* record : candidates)
				{
					if (MatchRecord(stream, *record, position, prefixBytes, finding))
					{
						return true;
					}
				}
			}

			++position;
		}

		return false;
	}

	inline bool ScanByteStream(IByteStream& stream, ObjectType objectType, const AvDatabase& db, ScanFinding& finding)
	{
		finding = {};
		if (stream.Size() < 8 || !stream.Seek(0))
		{
			return false;
		}

		const auto automaton = BuildPrefixAutomaton(db);
		std::size_t state = 0;
		BYTE current = 0;
		std::uint64_t position = 0;

		while (stream.ReadExact(&current, 1))
		{
			while (state != 0 && automaton[state].next.find(current) == automaton[state].next.end())
			{
				state = automaton[state].fail;
			}

			auto nextIt = automaton[state].next.find(current);
			if (nextIt != automaton[state].next.end())
			{
				state = nextIt->second;
			}

			if (!automaton[state].outputs.empty() && position + 1 >= 8)
			{
				for (std::uint64_t key : automaton[state].outputs)
				{
					auto recordIt = db.records.find(key);
					if (recordIt == db.records.end())
					{
						continue;
					}

					const std::uint64_t scanStart = position + 1 - 8;
					const std::array<BYTE, 8> prefixBytes = [] (std::uint64_t value)
					{
						std::array<BYTE, 8> bytes{};
						for (std::size_t i = 0; i < bytes.size(); ++i)
						{
							bytes[i] = static_cast<BYTE>((value >> (i * 8)) & 0xFF);
						}
						return bytes;
					}(key);

					for (const auto& record : recordIt->second)
					{
						if (record.objectType != objectType)
						{
							continue;
						}

						if (static_cast<std::int64_t>(scanStart) < record.offsetBegin || static_cast<std::int64_t>(scanStart) > record.offsetEnd)
						{
							continue;
						}

						if (MatchRecord(stream, record, scanStart, prefixBytes, finding))
						{
							return true;
						}
					}
				}
			}

			++position;
		}

		return false;
	}

	inline bool ScanFile(const std::wstring& path, const AvDatabase& db, ScanFinding& finding)
	{
		FileByteStream stream(path);
		if (!stream.Open())
		{
			return false;
		}

		ObjectType type = ObjectType::Pe;
		DetectObjectType(path, type);
		finding.matchedPath = path;
		return ScanByteStreamLinear(stream, type, db, finding);
	}

	inline void EnumerateFilesRecursively(const std::wstring& root, std::vector<std::wstring>& files)
	{
		WIN32_FIND_DATAW data{};
		std::wstring pattern = root;
		if (!pattern.empty() && pattern.back() != L'\\')
		{
			pattern += L'\\';
		}
		pattern += L"*";

		HANDLE find = FindFirstFileW(pattern.c_str(), &data);
		if (find == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
			{
				continue;
			}

			std::wstring full = root;
			if (!full.empty() && full.back() != L'\\')
			{
				full += L'\\';
			}
			full += data.cFileName;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				EnumerateFilesRecursively(full, files);
			}
			else
			{
				files.push_back(full);
			}
		} while (FindNextFileW(find, &data));

		FindClose(find);
	}

	inline std::wstring ScanFolder(const std::wstring& path, const AvDatabase& db, std::vector<ScanFinding>& hits)
	{
		std::vector<std::wstring> files;
		EnumerateFilesRecursively(path, files);
		for (const auto& file : files)
		{
			ScanFinding finding{};
			if (ScanFile(file, db, finding) && finding.infected)
			{
				hits.push_back(finding);
			}
		}

		std::wstring summary = L"Сканирование завершено: " + std::to_wstring(files.size()) + L" файлов";
		if (!hits.empty())
		{
			summary += L", угроз: " + std::to_wstring(hits.size());
		}
		else
		{
			summary += L", угроз не найдено";
		}
		return summary;
	}
}
