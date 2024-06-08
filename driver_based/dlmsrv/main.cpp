#include "io.h"

int main() {
	std::cout << "- Driver Load Monitor -\n\n";

    HANDLE hDriverPort;
    WCHAR PortName[] = L"\\FilterDlmPort";

	while (TRUE) {
        HRESULT Result = FilterConnectCommunicationPort(PortName, 0, NULL, 0, NULL, &hDriverPort);
        if (FAILED(Result)) {
            std::cerr << "[-] failed to connect to driver port." << std::endl;
            return FALSE;
        }

        SendPendingGet(hDriverPort);

        CloseHandle(hDriverPort);
	}

	return 0;
}