#pragma once

#include <windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "crypt32.lib")

namespace avstore
{
	inline std::wstring GetSelfDirectory()
	{
		wchar_t path[MAX_PATH]{};
		const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
		if (len == 0 || len >= MAX_PATH)
		{
			return L".";
		}

		std::wstring full(path, len);
		const size_t slash = full.find_last_of(L"\\/");
		if (slash != std::wstring::npos)
		{
			full.resize(slash);
		}
		return full;
	}

	inline std::wstring GetAvDataDirectory()
	{
		wchar_t programData[MAX_PATH]{};
		DWORD len = GetEnvironmentVariableW(L"ProgramData", programData, MAX_PATH);
		std::wstring base = len > 0 ? std::wstring(programData, len) : GetSelfDirectory();
		std::wstring dir = base + L"\\RbpoPz\\av";
		CreateDirectoryW((base + L"\\RbpoPz").c_str(), nullptr);
		CreateDirectoryW(dir.c_str(), nullptr);
		return dir;
	}

	inline std::filesystem::path GetPublicCertPath()
	{
		return std::filesystem::path(GetSelfDirectory()) / L"data" / L"public_cert.pem";
	}

	inline std::filesystem::path GetCurrentDatabasePath()
	{
		return std::filesystem::path(GetAvDataDirectory()) / L"current.avdb";
	}

	inline std::filesystem::path GetBackupDatabasePath()
	{
		return std::filesystem::path(GetAvDataDirectory()) / L"backup.avdb";
	}

	inline std::filesystem::path GetBundledDefaultDatabasePath()
	{
		return std::filesystem::path(GetSelfDirectory()) / L"data" / L"default.avdb";
	}

	inline std::string ReadFileUtf8(const std::filesystem::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			return {};
		}

		return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	}

	inline bool ExtractPemBlock(const std::string& pem, const char* label, std::string& der)
	{
		const std::string begin = std::string("-----BEGIN ") + label + "-----";
		const std::string end = std::string("-----END ") + label + "-----";
		const size_t start = pem.find(begin);
		const size_t finish = pem.find(end);
		if (start == std::string::npos || finish == std::string::npos || finish <= start)
		{
			return false;
		}

		std::string base64 = pem.substr(start + begin.size(), finish - start - begin.size());
		base64.erase(std::remove(base64.begin(), base64.end(), '\r'), base64.end());
		base64.erase(std::remove(base64.begin(), base64.end(), '\n'), base64.end());

		DWORD required = 0;
		if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, nullptr, &required, nullptr, nullptr))
		{
			return false;
		}

		der.assign(required, '\0');
		if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, reinterpret_cast<BYTE*>(der.data()), &required, nullptr, nullptr))
		{
			return false;
		}

		der.resize(required);
		return true;
	}

	class SignatureVerifier
	{
	public:
		bool Initialize(const std::filesystem::path& certPath = GetPublicCertPath())
		{
			Release();

			const std::string pem = ReadFileUtf8(certPath);
			std::string certDer;
			if (!ExtractPemBlock(pem, "CERTIFICATE", certDer))
			{
				return false;
			}

			if (!CryptAcquireContextW(&m_prov, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
			{
				return false;
			}

			PCCERT_CONTEXT certContext = CertCreateCertificateContext(
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				reinterpret_cast<const BYTE*>(certDer.data()),
				static_cast<DWORD>(certDer.size()));
			if (!certContext)
			{
				Release();
				return false;
			}

			const BOOL imported = CryptImportPublicKeyInfo(
				m_prov,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				&certContext->pCertInfo->SubjectPublicKeyInfo,
				&m_key);
			CertFreeCertificateContext(certContext);
			if (!imported)
			{
				Release();
				return false;
			}

			m_ready = true;
			return true;
		}

		bool Verify(const std::vector<BYTE>& data, const std::vector<BYTE>& signature) const
		{
			if (!m_ready || signature.empty())
			{
				return false;
			}

			HCRYPTHASH hash = 0;
			if (!CryptCreateHash(m_prov, CALG_SHA_256, 0, 0, &hash))
			{
				return false;
			}

			if (!CryptHashData(hash, data.data(), static_cast<DWORD>(data.size()), 0))
			{
				CryptDestroyHash(hash);
				return false;
			}

			const BOOL ok = CryptVerifySignatureW(
				hash,
				const_cast<BYTE*>(signature.data()),
				static_cast<DWORD>(signature.size()),
				m_key,
				nullptr,
				0);

			CryptDestroyHash(hash);
			return ok == TRUE;
		}

		~SignatureVerifier()
		{
			Release();
		}

	private:
		void Release()
		{
			if (m_key)
			{
				CryptDestroyKey(m_key);
				m_key = 0;
			}
			if (m_prov)
			{
				CryptReleaseContext(m_prov, 0);
				m_prov = 0;
			}
			m_ready = false;
		}

		HCRYPTPROV m_prov = 0;
		HCRYPTKEY m_key = 0;
		bool m_ready = false;
	};
}
