#include "io.h"

int main() {
	std::cout << "test\n";

    HANDLE hDriverPort;
    WCHAR PortName[] = L"\\FilterDlmPort";

	while (TRUE) {
        HRESULT Result = FilterConnectCommunicationPort(PortName, 0, NULL, 0, NULL, &hDriverPort);
        if (FAILED(Result)) {
            std::cout << "[*] failed to connect to driver port" << std::endl;
            return FALSE;
        }

        SendPendingGet(hDriverPort);

        CloseHandle(hDriverPort);
	}

	return 0;
}