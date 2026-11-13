#pragma once
#include <Windows.h>
#include <string>

typedef LONG NTSTATUS, * PNTSTATUS;
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

namespace M_base
{
	namespace Misc
	{
		int RandomInt(int Lower = -100, int Max = 100);
		std::wstring RandomString(size_t Length = 12);
		std::string Wstring2String(std::wstring Input);
		std::wstring String2WString(std::string Input);
       
		void InitializeConsole();
		void ShutDownConsole();
		std::string DownloadServerInfo(std::wstring IP, int PORT);
		bool GetServerIDFromHeaders(const std::wstring& IP, int PORT);
		std::string GetServerInfoFromID();
		std::string DownloadServerInfoFromUrl(const std::string& url);
		std::string GetServerID();

		void OptimizeThreadForGameSync();

		class PrecisionPacer {
		public:
			PrecisionPacer();
			~PrecisionPacer();
			void SleepMicros(long long micros);
		private:
			HANDLE hTimer;
		};
	}
}