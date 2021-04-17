// APT27-lucky-variant.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>


#include <Windows.h>




#define NOT_EXIST			0x00 
#define EXIST				0x01
#define NEED_RESTART		0x02
#define SEARCH_REG_BLB		0x04
#define WARNING				0x08

HANDLE hConsole;
UINT warn = 0;




class registry_key {

public:
	registry_key(const char reg_key[]) {
		SplitRoot(std::string(reg_key));
		value_count = 0;
	}


private:
	HKEY key;
	std::string root_key;
	std::string subkey;
	int value_count;
	std::vector<std::string> value;
	std::vector<std::string> data;

	bool SplitRoot(std::string path) {
		
		int pos;
		if (!(pos = path.find("\\")))
			return 0;

		std::string root = path.substr(0, pos);
		key = NULL;
		if (root == "hklm" || root == "HKLM") {
			root_key = "HKLM";
			key = HKEY_LOCAL_MACHINE;
		}if (root == "hkcu" || root == "HKCU") {
			root_key = "HKCU";
			key = HKEY_CURRENT_USER;
		}if (root == "hkcr" || root == "HKCR") {
			root_key = "HKCR";
			key = HKEY_CLASSES_ROOT;
		}if (root == "hkcc" || root == "HKCC") {
			root_key = "HKCC";
			key = HKEY_CURRENT_CONFIG;
		}

		if (key != NULL)
			subkey = path.substr(pos + 1);
		else
			return 0;

		return 1;

	}


public:
	void set_key(std::string in) { subkey = in; }
	
	std::string get_key() { return subkey; }

	HKEY get_root() { return key; }

	std::string get_registry() { return std::string(root_key + "\\" + subkey); }
	
	int add_value(std::string val,std::string val_data) {
		value_count++;
		value.push_back(val);
		data.push_back(val_data);

		return value_count;
	}

};


#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

void EnumKey(HKEY root,std::wstring subkey)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	HKEY hkey = nullptr;
	if (RegOpenKeyEx(root, subkey.c_str(), 0, KEY_READ, &hkey) != ERROR_SUCCESS)
		return;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hkey,      // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 


	// Enumerate the subkeys, until RegEnumKeyEx fails.

	if (cSubKeys)
	{

		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hkey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				EnumKey(root, subkey + std::wstring((TCHAR*)achKey)+L"\\");
			}
		}
	}


	// Enumerate the key values. 



	ULONG TotalSize = 0;

	if (cValues && cbMaxValueData)
	{
		//printf("\nNumber of values: %d\n", cValues);
		BYTE* buffer = new BYTE[cbMaxValueData];
		ZeroMemory(buffer, cbMaxValueData);

		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = RegEnumValue(hkey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);

			if (retCode == ERROR_SUCCESS)
			{

				DWORD lpData = cbMaxValueData;
				buffer[0] = '\0';
				DWORD ValueType;
				LONG dwRes = RegQueryValueEx(hkey, achValue, 0, &ValueType, buffer, &lpData);
				if (ValueType == REG_BINARY)
				{
					TotalSize += lpData;
				}
				//_tprintf(TEXT("(%d) %s : %s\n"), i + 1, achValue, buffer);
			}
		}

		delete[] buffer;
	}

	if (TotalSize >= 5000)
	{
		std::wcout << L"Suspicious registry BLOB has been seen: '" << subkey << L"'.\n";
		warn++;

	}


}



bool Enumerate_REG_BLOB(registry_key reg) {

	std::string tmp = reg.get_key();
	EnumKey(reg.get_root(), std::wstring( tmp.begin(),tmp.end() ) );


	return false;
}




class Infection {


public:
	Infection(std::vector<registry_key> reg_list, std::vector<USHORT> reg_state,std::vector<std::string> file_list, std::vector<USHORT> file_state ) {
		Regs = reg_list;
		Files = file_list;
		Regs_state = reg_state;
		Files_state = file_state;
	};

	int CheckForInfection() {

		int count = 0;

		for (int i = 0; i < Regs.size(); i++)
		{
			HKEY subKey = nullptr;
			
			if (Regs_state[i] & SEARCH_REG_BLB)
			{
				Enumerate_REG_BLOB(Regs[i]);
			}
			else
			{
				if (RegOpenKeyExA(Regs[i].get_root(), Regs[i].get_key().c_str(), 0, KEY_READ, &subKey) == ERROR_SUCCESS)
				{
					Regs_state[i] |= EXIST;

					if (Regs_state[i] & WARNING)
					{
						std::cout << "Suspicious registry artifact has been seen: '" << Regs[i].get_registry().c_str() << "'.\n";
						warn++;
					}
					else
					{
						std::cout << "Registry IOC has been seen: '" << Regs[i].get_registry().c_str() << "'.\n";
						count++;
					}
				}
				else
				{
					Regs_state[i] |= NOT_EXIST;
				}
			}
		}

	
		for (int i = 0; i < Files.size(); i++)
		{
			if (std::filesystem::exists( Files[i].c_str() ) )
			{
				Files_state[i] |= EXIST;
				

				if (Files_state[i] & WARNING)
				{
					std::cout << "Suspicious File/Directory artifact has been seen: '" << Files[i].c_str() << "'.\n";
					warn++;
				}
				else
				{
					std::cout << "File/Directory IOC has been seen: '" << Files[i].c_str() << "'.\n";
					count++;
				}

			}
			else
				Files_state[i] |= NOT_EXIST;


		}

		return count;
		
	}

	int Disinfect()
	{
		int not_fully_cleaned = 0;
		bool restart = false;


		for (int i = 0; i < Regs.size(); i++)
		{
			if ( (Regs_state[i] & EXIST) && !(Regs_state[i] & WARNING) )
			{
				if (RegDeleteTreeA(Regs[i].get_root(), Regs[i].get_key().c_str()) == ERROR_SUCCESS)
				{
					std::cout << "Removed : '" << Regs[i].get_registry().c_str() << "' \n";
					if (Regs_state[i] & NEED_RESTART)
						restart = true;
				}
				else
					not_fully_cleaned++;


			}
		}

		for (int i = 0; i < Files.size(); i++)
		{
			if ( (Files_state[i] & EXIST) && !(Files_state[i] & WARNING) )
			{
				if (std::filesystem::remove_all(Files[i].c_str()))
				{
					std::cout << "Removed : '" << Files[i].c_str() << "' \n";
					if (Files_state[i] & NEED_RESTART)
						restart = true;
				}
				else
					not_fully_cleaned++;

			}

		}

		if (restart) {
			SetConsoleTextAttribute(hConsole, 0x0006|FOREGROUND_INTENSITY);
			std::cout << "\nATTENTION: Disinfection process needs restarting the system. Please RESTART the system and re-run this removal ...\n";
			SetConsoleTextAttribute(hConsole, 0x000f);
		}
		else if (!not_fully_cleaned) {
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
			std::cout << "System is fully disinfected\n";
			SetConsoleTextAttribute(hConsole, 0x000f);
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 0x0006);
			std::cout << "Some of the indicators can not be cleaned from the system. Please contact Graph Inc. support\n";
			SetConsoleTextAttribute(hConsole, 0x000f);
		}
		return 0;
	}


private:
	std::vector <registry_key> Regs;
	std::vector<USHORT> Regs_state;
	std::vector<std::string> Files;
	std::vector<USHORT> Files_state;

};







int main()
{
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 0x000f);

	std::cout << "###################################################################\n";
	std::cout << "###\t\t" << "APT 27 New Variant Detector & Removal" << "\t\t###\n";
	std::cout << "###\t\t" << "Provided by Graph Inc.(graph-inc.ir)" << "\t\t###\n";
	std::cout << "###\tFor ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	std::cout << "URGENT";
	SetConsoleTextAttribute(hConsole, 0x000f);
	std::cout << " Incident Response service requests, call \t###\n";
	std::cout << "###\t\t+ 9821 47 71 6666 (Iran) or send email" << "\t\t###\n";
	std::cout << "###\t\tto mdr@graph-inc.ir(other countries)" << "\t\t###\n";
	
	std::cout << "###################################################################\n";

	std::cout << "Press enter to start detector ...\n";

	std::cin.ignore();


	Infection APT27_lucky(	std::vector<registry_key>{	registry_key("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wdf02000\\"),
														registry_key("HKLM\\SOFTWARE\\Classes\\wds2s2update\\"),
														registry_key("HKLM\\SOFTWARE\\Classes\\vmware_up\\"),
														registry_key("HKLM\\SOFTWARE\\Classes\\")},
							std::vector<USHORT>{ NOT_EXIST, NEED_RESTART, NOT_EXIST,WARNING|SEARCH_REG_BLB },
							std::vector<std::string> { 	"C:\\Windows\\Security\\capexex\\",
														"C:\\Windows\\Security\\capex\\",
														"C:\\Windows\\setup\\setup.exe",
														"C:\\Windows\\SchCache\\setup.exe",
														"C:\\Windows\\Speech\\setup.exe",
														"C:\\ProgramData\\x.log",
														"C:\\ProgramData\\sw.log"},
							std::vector<USHORT> { NOT_EXIST, NOT_EXIST, NOT_EXIST, NOT_EXIST, NOT_EXIST, WARNING, WARNING }
		);


	std::cout << "Please wait. The detector is searching for malicious signs...\n\n";
	if ( APT27_lucky.CheckForInfection() )
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		std::cout << "System is Infected\n";
		SetConsoleTextAttribute(hConsole, 0x000f);
		std::cout << "Press enter to continute to disinfect ...\n";

		std::cin.ignore();

		APT27_lucky.Disinfect();
	}
	else if (warn) {
		SetConsoleTextAttribute(hConsole, 0x0006);
		std::cout << "\nPossible suspicious indecators have been detected. Please contact Graph for further confirmation.\n";
		SetConsoleTextAttribute(hConsole, 0x000f);
	}
	else
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
		std::cout << "\nSystem is not Infected\n";
		SetConsoleTextAttribute(hConsole, 0x000f);
	}

	std::cout << "\nPress enter to exit ...";
	std::cin.ignore();
	
	return 0;
}

