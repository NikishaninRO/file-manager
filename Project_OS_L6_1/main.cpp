#include <Windows.h>
#include <string>

LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM); // оконная функция

HINSTANCE hinstance;

HWND w; //Ссылка на окно, позволяет однозначно определить каждое окно
HWND hComboBox_Driver_Directories_Files; // список содержимого выбранного диска по указанному пути
HWND hStatic_INFO; // дескриптор статической области для вывода информации об объекте(диск/файл/каталог)

HANDLE hFile; // дескриптор специального файла, куда помещается информация о выбранном диске/файле/каталоге

std::wstring cur_path_buf; // строка-буффер для хранения текущего пути, где находится пользователь в в файловой системе

void update_combobox_dirs_files(); // обновить содержимое списка, где храниться содержимое по текущему пути в файловой системе
void get_driver_info(std::wstring& disk); // получение иформации о диске по его имени
void get_dir_file_info(std::wstring& file_dir); // получение иформации о файле/каталоге по расположению
BOOL is_file(std::wstring& path); // определить, чем является выделенный элемент(файлом или каталогом) по переданному пути

int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	hinstance = GetModuleHandleW(NULL); //возвращаем дескриптор программы

	// создаем и регистрируем класс главного окна
	WNDCLASSEX wclass = { 0 };
	wclass.cbSize = sizeof(WNDCLASSEX);
	wclass.style = CS_HREDRAW | CS_VREDRAW; //Стиль окна класса: окно будет перерисовываться при изменении его горизонтальных и вертикальных размеров
	wclass.lpfnWndProc = WndProc;  //Указатель на оконную функцию, которая будет обрабатывать все сообщения, посылаемые окну  
	wclass.cbClsExtra = 0; //Выделенная память, используемая программой по своему усмотрению
	wclass.cbWndExtra = 0; //Выделенная память, используемая программой по своему усмотрению
	wclass.hInstance = hinstance;  //Ссылка на экземпляр программы, используется ядром Windows для однозначного определения сегмента данных экземпляра программы
	wclass.hIcon = LoadIcon(0, IDI_APPLICATION); //Ссылка на иконку для окна, для отображения минимизированного окна, в данном случае – иконка, соответствующая приложению
	wclass.hCursor = LoadCursor(0, IDC_ARROW);   //Ссылка на курсор, в данном случае – в виде стрелки
	wclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);   //Ссылка на шаблон заполнения фона для окна 
	wclass.lpszMenuName = NULL;    //Ссылка на строку с именем меню
	wclass.lpszClassName = L"FORM";
	RegisterClassEx(&wclass);

	HWND w; //Ссылка на окно, позволяет однозначно определить каждое окно

	w = CreateWindowExW( //Создаем главное окно и возвращаем ссылку на окно типа HWND
		0,
		L"FORM",     //Имя класса, к которому принадлежит создаваемое окно           
		L"Файловый менеджер",   //Заголовок окна             
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, //Стиль окна, в данном случае – комбинация стилей
		CW_USEDEFAULT, //X – начальная позиция верхнего левого угла, в данном случае – значение по умолчанию
		CW_USEDEFAULT, //Y – начальная позиция верхнего левого угла, в данном случае – значение по умолчанию
		1250, //Width – начальная ширина окна, в данном случае – значение по умолчанию
		550, //Height – начальная ширина окна, в данном случае – значение по умолчанию
		0,   //WndParent – родительское окно данного окна    
		0,   //Menu – меню, используемое данным окном    
		hinstance, //hInstance – указывает на экземпляр программы, этот параметр указывается, чтобы оконная функция имела доступ к сегменту данных программы
		0 //lpParam – определяет дополнительную информацию, посылаемую через сообщение wm_Create
	);

	//Окно создано, его необходимо отобразить на экране
	ShowWindow(w, SW_SHOW); //Отображает или делает невидимым указанное окно

	UpdateWindow(w); //Указывает прикладной программе, что часть окна нуждается в перерисовке

	MSG msg = { 0 }; // Сообщение – реакция ядра Windows на какое-либо событие

	/*После того, как окно отбражено на экране, управление передаётся циклу обработки сообщений.
   GetMessage извлекает сообщения из очереди и помещается в структуру типа MSG.
   Для всех сообщений, отличных от WM_QUIT (завершение работы программы), эта функция возвращает ненулевое значение, и цикл продолжает обработку сообщений}*/
	while (GetMessage(&msg, 0, 0, 0)) { // GetMessage возвращает сообщение из очереди GetMessagePos
		//TranslateMessage передает структуру типа MSG ядру Windows для преобразования сообщений о введенных символах
		TranslateMessage(&msg); //Переводит сообщение виртуальных клавиш в символьное сообщение
		DispatchMessage(&msg); //Передает сообщение оконной функции указанного окна

		//После того, как оконная функция обработала сообщение, управление возвращается в цикл обработки сообщений
	} //конец цикла обработки сообщений

	return 0; //Программа завершается
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hComboBox_Drivers; // дескриптор списка доступных в системе дисков

	static HWND hButton_GET_DRIVER_INFO; // дескриптор кнопки "Получить информацию о выбранном диске"
	static HWND hButton_GET_FILE_DIR_INFO; // дескриптор кнопки "Получить информацию о выбранном файле/каталоге"

	static HWND hButton_OPEN_DRIVER; // дескриптор кнопки "Открыть содержимое выбранного диска"
	static HWND hButton_RIGHT; // дескриптор кнопки "->"
	static HWND hButton_LEFT; // дескриптор кнопки "<-"
	
	static HWND hButton_RENAME; // дескриптор кнопки "Переименовать выбранный файл/каталог"
	static HWND hButton_CONFIRM_RENAME; // дескриптор кнопки "Подтвердить ввод" (для переименования)
	static HWND hButton_CANCEL_RENAME; // дескриптор кнопки "Отмена" (для переименования)

	static HWND hButton_MOVE; // дескриптор кнопки "Переместить выбранный файл/каталог"
	static HWND hButton_CONFIRM_MOVE; // дескриптор кнопки "Подтвердить ввод" (для перемещения)
	static HWND hButton_CANCEL_MOVE; // дескриптор кнопки "Отмена" (для перемещения)

	static HWND hButton_CREATE; // дескриптор кнопки "Создать новый файл/каталог"
	static HWND hButton_CONFIRM_CREATE; // дескриптор кнопки "Подтвердить ввод" (для создания)
	static HWND hButton_CANCEL_CREATE; // дескриптор кнопки "Отмена" (для создания)

	static HWND hButton_COPY; // дескриптор кнопки "Создать копию выбранного файла/каталога"
	static HWND hButton_CONFIRM_COPY; // дескриптор кнопки "Подтвердить ввод" (для копирования)
	static HWND hButton_CANCEL_COPY; // дескриптор кнопки "Отмена" (для копирования)

	static HWND hButton_DELETE; // дескриптор кнопки "Удалить выбранный файл/каталог"

	static HWND hEdit_RENAME; // дескриптор поля для переименовывания файла/каталога
	static HWND hEdit_MOVE; // дескриптор поля для перемещения файла/каталога
	static HWND hEdit_CREATE; // дескриптор поля для создания нового файла/каталога
	static HWND hEdit_COPY; // дескриптор поля для создания копии выбранного файла/каталога
	
	// Здесь указывается реакция оконной функции на сообщения Windows:
	switch (msg) {
	case WM_CREATE:

		// создаем кнопки для перемещения в файловой системе
		hButton_OPEN_DRIVER = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Открыть выбранный диск",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			55,         // y position 
			180,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_LEFT = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"<-",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			880,         // x position 
			30,         // y position 
			40,        // Button width
			20,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_RIGHT = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"->",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			930,         // x position 
			30,         // y position 
			40,        // Button width
			20,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// объекты для выполнения различных операций над файлами/каталогами

		// переименование

		hButton_RENAME = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Переименовать выбранный файл/каталог",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			90,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CONFIRM_RENAME = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Подтвердить ввод",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			950,         // x position 
			90,         // y position 
			150,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CANCEL_RENAME = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Отмена",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			1110,         // x position 
			90,         // y position 
			100,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// создаем поле редактирования
		hEdit_RENAME = CreateWindowEx(0, L"edit", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE, 340, 95, 600, 20,
			hWnd, 0, hinstance, NULL);
		EnableWindow(hEdit_RENAME, FALSE);

		// перемещение

		hButton_MOVE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Переместить выбранный файл/каталог",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			140,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CONFIRM_MOVE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Подтвердить ввод",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			950,         // x position 
			140,         // y position 
			150,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CANCEL_MOVE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Отмена",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			1110,         // x position 
			140,         // y position 
			100,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// создаем поле редактирования
		hEdit_MOVE = CreateWindowEx(0, L"edit", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE, 340, 145, 600, 20,
			hWnd, 0, hinstance, NULL);
		EnableWindow(hEdit_MOVE, FALSE);

		// создание

		hButton_CREATE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Создать новый файл/каталог",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			190,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CONFIRM_CREATE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Подтвердить ввод",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			950,         // x position 
			190,         // y position 
			150,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CANCEL_CREATE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Отмена",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			1110,         // x position 
			190,         // y position 
			100,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// создаем поле редактирования
		hEdit_CREATE = CreateWindowEx(0, L"edit", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE, 340, 195, 600, 20,
			hWnd, 0, hinstance, NULL);
		EnableWindow(hEdit_CREATE, FALSE);

		// копирование

		hButton_COPY = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Создать копию выбранного файла/каталога",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			240,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CONFIRM_COPY = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Подтвердить ввод",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			950,         // x position 
			240,         // y position 
			150,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.
		hButton_CANCEL_COPY = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Отмена",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			1110,         // x position 
			240,         // y position 
			100,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// создаем поле редактирования
		hEdit_COPY = CreateWindowEx(0, L"edit", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE, 340, 245, 600, 20,
			hWnd, 0, hinstance, NULL);
		EnableWindow(hEdit_COPY, FALSE);

		// удаление

		hButton_DELETE = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Удалить выбранный файл/каталог",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			30,         // x position 
			290,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// кнопки для вывода информации о выделенных дисках/файлах/каталогах

		hButton_GET_DRIVER_INFO = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Получить информацию о выбранном диске",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			350,         // x position 
			290,         // y position 
			300,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		hButton_GET_FILE_DIR_INFO = CreateWindowEx(
			0,
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Получить информацию о выбранном файле/каталоге",  // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
			670,         // x position 
			290,         // y position 
			400,        // Button width
			30,        // Button height
			hWnd,     // Parent window
			NULL,       // No menu.
			hinstance,
			NULL);      // Pointer not needed.

		// создаем список для доступных дисков
		hComboBox_Drivers = CreateWindowEx(0L, L"ComboBox", NULL,
			WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
			30, 30, 180, 250,
			hWnd, (HMENU)100, hinstance, NULL);
		
		// заполняем список дисков содержимым
		{
			TCHAR disks[256];
			TCHAR* disk;
			GetLogicalDriveStringsW(256, disks);
			disk = disks;
			while (*disk)
			{
				SendMessage(hComboBox_Drivers, CB_ADDSTRING, (WPARAM)0, (LPARAM)disk);
				disk = disk + wcslen(disk) + 1;
			}
		}

		SendMessage(hComboBox_Drivers, CB_SETCURSEL, (WPARAM)0, (LPARAM)0); // выделяем первый диск из списка

		// создаем список для содержимого выбранного диска
		hComboBox_Driver_Directories_Files = CreateWindowEx(0L, L"ComboBox", NULL,
			WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
			250, 30, 620, 250,
			hWnd, (HMENU)200, hinstance, NULL);

		// создаем статическое поле
		hStatic_INFO = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE,
			30, 350, 700, 150, hWnd, 0, hinstance, NULL);

		// создаем специальный файл для вывода информации о выбранном диске/файле/каталоге
		hFile = CreateFile(L"E:\\OUTPUT_INFO.txt", GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
		CloseHandle(hFile);

		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc{ 0 };  // ссылка на контекст, должна использоваться для работы с окном 
			hdc = BeginPaint(hWnd, &ps); // начинаем рисовать
			// делаем подписи к спискам и полям
			TCHAR text_1[100] = L"Доступные диски системы:";
			TextOut(hdc, 30, 10, text_1, ARRAYSIZE(text_1)); // выводим строку на текущее окно
			TCHAR text_2[150] = L"";
			wsprintf(text_2, L"Путь: %s", cur_path_buf.c_str());
			TextOut(hdc, 250, 10, text_2, ARRAYSIZE(text_2)); // выводим строку на текущее окно
			TCHAR text_3[100] = L"Новое имя файла/каталога:";
			TextOut(hdc, 340, 70, text_3, ARRAYSIZE(text_3)); // выводим строку на текущее окно
			TCHAR text_4[100] = L"Новый полный путь файла/каталога:";
			TextOut(hdc, 340, 120, text_4, ARRAYSIZE(text_4)); // выводим строку на текущее окно
			TCHAR text_5[100] = L"Полный путь нового файла/каталога(для файла обязательно укажите расширение):";
			TextOut(hdc, 340, 170, text_5, ARRAYSIZE(text_5)); // выводим строку на текущее окно
			TCHAR text_6[100] = L"Полный путь копии файла/каталога:";
			TextOut(hdc, 340, 220, text_6, ARRAYSIZE(text_6)); // выводим строку на текущее окно
			TCHAR text_7[100] = L"Информация о последнем открытом объекте(диск/файл/каталог):";
			TextOut(hdc, 30, 330, text_7, ARRAYSIZE(text_7)); // выводим строку на текущее окно
			EndPaint(hWnd, &ps); // заканчиваем рисовать
		}
		break;
	case WM_COMMAND:

		// открытие диска

		if ((HWND)lParam == hButton_OPEN_DRIVER) // выводим в список содержимое выбранного диска
		{
			std::wstring cur_str, info_buf;
			int ind = (int)SendMessage(hComboBox_Drivers, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Drivers, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str.c_str()); // получаем выделенную строку
			cur_path_buf = cur_str.c_str();
			update_combobox_dirs_files(); // обновляем список файлов/каталогов
		}

		// перемещение по диску

		if ((HWND)lParam == hButton_LEFT) // откатываемся на один элемент вложенного пути
		{
			if (cur_path_buf.size() <= 3)
			{
				cur_path_buf = L"";
				InvalidateRect(w, NULL, TRUE); // перерисовывем окно
				SendMessage(hComboBox_Driver_Directories_Files, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0); // удаляем текущее содержимое списка
			}
			else
			{
				bool second_slesh = false;
				for (int i = cur_path_buf.size() - 1; i >= 0; --i)
				{
					if (cur_path_buf[i] == L'\\') // встречен символ - разделитель
					{
						if (second_slesh) // он является вторым с конца строки
						{
							cur_path_buf.resize(i + 1);
							break;
						}
						else
						{
							second_slesh = true;
						}
					}
				}
				update_combobox_dirs_files(); // обновляем список файлов/каталогов
			}
		}
		if ((HWND)lParam == hButton_RIGHT) // переходим в выделенный элемент для просмотра содержимого, если он является каталогом
		{
			TCHAR cur_str[100] = L"";
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку
			
			WIN32_FIND_DATA FindFileData; // структура для хранения информации и найденном файле/каталоге
			HANDLE hFind; // дескриптор поиска
			std::wstring temp = cur_path_buf;
			temp += cur_str;
			hFind = FindFirstFile(temp.c_str(), &FindFileData); // находим элемент по выделенной строке
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // выделен каталог
			{
				cur_path_buf = temp.c_str();
				cur_path_buf += '\\';
				update_combobox_dirs_files(); // обновляем список файлов/каталогов
			}
			FindClose(hFind); // закрываем дескриптор поиска
		}

		// получение информации о дисках/файлах/каталогах
		
		if ((HWND)lParam == hButton_GET_DRIVER_INFO) // получаем информацию о диске и выводим ее на форму и в специальный файл
		{
			std::wstring cur_str;
			int ind = (int)SendMessage(hComboBox_Drivers, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			if (ind >= 0)
			{
				(TCHAR)SendMessage(hComboBox_Drivers, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str.c_str()); // получаем выделенную строку
				get_driver_info(cur_str);
			}
			else
			{
				MessageBox(hWnd, L"Диск не выбран!", L"Внимание!", MB_OK);
			}
		}
		if ((HWND)lParam == hButton_GET_FILE_DIR_INFO) // получаем информацию о файле/каталоге и выводим ее на форму и в специальный файл
		{
			std::wstring path = cur_path_buf;
			TCHAR cur_str[100];
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			if (ind >= 0)
			{
				(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку
				path += cur_str;
				get_dir_file_info(path);
			}
			else
			{
				MessageBox(hWnd, L"Файл/каталог не выбран!", L"Внимание!", MB_OK);
			}
		}

		// операция переименования
		
		if ((HWND)lParam == hButton_RENAME) // переименовываем выбранный файл/каталог
		{
			EnableWindow(hEdit_RENAME, TRUE); // делаем поле редактирования активным
			SetFocus(hEdit_RENAME); // устанавливаем фокус на поле редактирования
		}
		if ((HWND)lParam == hButton_CONFIRM_RENAME) // подтверждаем переименовывание
		{
			std::wstring temp = cur_path_buf, old_name, new_name;
			TCHAR edit_info[100] = L"";

			TCHAR cur_str[100] = L"";
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку

			GetWindowText(hEdit_RENAME, edit_info, 100); // получаем содержимое поля редактирования
			
			// получаем полные пути файла со старым и новым именем
			old_name += temp;
			old_name += cur_str;
			new_name += temp;
			new_name += edit_info;

			MoveFile(old_name.c_str(), new_name.c_str()); // переименовываем файл/каталог
			update_combobox_dirs_files(); // обновляем список файлов/каталогов

			SetWindowText(hEdit_RENAME, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_RENAME, FALSE); // делаем поле редактирования неактивным
		}
		if ((HWND)lParam == hButton_CANCEL_RENAME) // отменяем переименовывание
		{
			SetWindowText(hEdit_RENAME, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_RENAME, FALSE); // делаем поле редактирования неактивным
		}

		// операция перемещения

		if ((HWND)lParam == hButton_MOVE) // перемещаем выбранный файл/каталог
		{
			EnableWindow(hEdit_MOVE, TRUE); // делаем поле редактирования активным
			SetFocus(hEdit_MOVE); // устанавливаем фокус на поле редактирования
		}
		if ((HWND)lParam == hButton_CONFIRM_MOVE) // подтверждаем перемещение
		{
			std::wstring temp = cur_path_buf, old_path;
			TCHAR new_path[150] = L"";

			TCHAR cur_str[100] = L"";
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку

			GetWindowText(hEdit_MOVE, new_path, 150); // получаем содержимое поля редактирования

			// получаем полные пути файла со старым и новым именем
			old_path += temp;
			old_path += cur_str;

			MoveFile(old_path.c_str(), new_path); // перемещаем файл/каталог
			update_combobox_dirs_files(); // обновляем список файлов/каталогов

			SetWindowText(hEdit_MOVE, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_MOVE, FALSE); // делаем поле редактирования неактивным
		}
		if ((HWND)lParam == hButton_CANCEL_MOVE) // отменяем переименовывание
		{
			SetWindowText(hEdit_MOVE, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_MOVE, FALSE); // делаем поле редактирования неактивным
		}

		// операция создания

		if ((HWND)lParam == hButton_CREATE) // создаем файл/каталог
		{
			EnableWindow(hEdit_CREATE, TRUE); // делаем поле редактирования активным
			SetFocus(hEdit_CREATE); // устанавливаем фокус на поле редактирования
		}
		if ((HWND)lParam == hButton_CONFIRM_CREATE) // подтверждаем создание
		{
			std::wstring cur_str;
			TCHAR new_elem_path[150] = L"";

			GetWindowText(hEdit_CREATE, new_elem_path, 150); // получаем содержимое поля редактирования
			cur_str = new_elem_path;

			if (is_file(cur_str))
			{
				CreateFile(new_elem_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0); // создаем новый файл
			}
			else
			{
				CreateDirectory(new_elem_path, NULL); // создаем новый каталог
			}
			
			update_combobox_dirs_files(); // обновляем список файлов/каталогов

			SetWindowText(hEdit_CREATE, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_CREATE, FALSE); // делаем поле редактирования неактивным
		}
		if ((HWND)lParam == hButton_CANCEL_CREATE) // отменяем создание
		{
			SetWindowText(hEdit_CREATE, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_CREATE, FALSE); // делаем поле редактирования неактивным
		}

		// операция копирования

		if ((HWND)lParam == hButton_COPY) // создаем копию файла/каталога
		{
			EnableWindow(hEdit_COPY, TRUE); // делаем поле редактирования активным
			SetFocus(hEdit_COPY); // устанавливаем фокус на поле редактирования
		}
		if ((HWND)lParam == hButton_CONFIRM_COPY) // подтверждаем копирование
		{
			std::wstring temp = cur_path_buf;
			std::wstring copy;
			TCHAR copy_path[150] = L"";

			TCHAR cur_str[100] = L"";
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку

			GetWindowText(hEdit_COPY, copy_path, 150); // получаем содержимое поля редактирования

			temp += cur_str;
			copy = copy_path;

			if (is_file(temp))
			{
				CopyFile(temp.c_str(), copy_path, TRUE); // копируем выбранный файл
			}
			else
			{
				TCHAR *tmp = (TCHAR*)temp.c_str();
				TCHAR *cpy = (TCHAR*)copy.c_str();
				tmp[lstrlen(tmp) + 1] = L'\0';
				cpy[lstrlen(cpy) + 1] = L'\0';
				SHFILEOPSTRUCT dir_struct; // структура для хранения файловый операций
				ZeroMemory(&dir_struct, sizeof(dir_struct));
				dir_struct.hwnd = NULL;
				dir_struct.wFunc = FO_COPY;
				dir_struct.pFrom = tmp;
				dir_struct.pTo = cpy;
				dir_struct.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMMKDIR | FOF_RENAMEONCOLLISION;
				dir_struct.fAnyOperationsAborted = FALSE;
				dir_struct.hNameMappings = 0;
				dir_struct.lpszProgressTitle = NULL;
				SHFileOperation(&dir_struct); // копируем выбранный каталог вместе с содержимым
			}

			update_combobox_dirs_files(); // обновляем список файлов/каталогов

			SetWindowText(hEdit_COPY, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_COPY, FALSE); // делаем поле редактирования неактивным
		}
		if ((HWND)lParam == hButton_CANCEL_COPY) // отменяем копирование
		{
			SetWindowText(hEdit_COPY, L""); // устанавливаем новое содержимое поля редактирования
			EnableWindow(hEdit_COPY, FALSE); // делаем поле редактирования неактивным
		}

		// операция удаления

		if ((HWND)lParam == hButton_DELETE) // удаляем выбранный файл/каталог
		{
			std::wstring temp = cur_path_buf;

			TCHAR cur_str[100] = L"";
			int ind = (int)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // получаем индекс выделенной строки
			(TCHAR)SendMessage(hComboBox_Driver_Directories_Files, (UINT)CB_GETLBTEXT, (WPARAM)ind, (LPARAM)cur_str); // получаем выделенную строку

			temp += cur_str;

			if (is_file(temp))
			{
				DeleteFile(temp.c_str()); // удаляем выбранный файл
			}
			else
			{
				TCHAR* tmp = (TCHAR*)temp.c_str();
				tmp[lstrlen(tmp) + 1] = L'\0';
				SHFILEOPSTRUCT dir_struct; // структура для хранения файловый операций
				ZeroMemory(&dir_struct, sizeof(dir_struct));
				dir_struct.hwnd = NULL;
				dir_struct.wFunc = FO_DELETE;
				dir_struct.pFrom = tmp;
				dir_struct.pTo = NULL;
				dir_struct.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMMKDIR;
				dir_struct.fAnyOperationsAborted = FALSE;
				dir_struct.hNameMappings = 0;
				dir_struct.lpszProgressTitle = NULL;
				SHFileOperation(&dir_struct); // удаляем выбранный каталог вместе с содержимым
			}

			update_combobox_dirs_files(); // обновляем список файлов/каталогов
		}

		break;
	case WM_CLOSE:
		DeleteFile(L"E:\\OUTPUT_INFO.txt"); // удаляем файл для вывода информации о дисках/файлах/каталогах
		break;
	case WM_DESTROY:
		PostQuitMessage(0); // Посылает прикладной программе сообщение wm_Quit код 0 – успешное завершение
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void update_combobox_dirs_files()
{
	InvalidateRect(w, NULL, TRUE); // перерисовывем окно
	SendMessage(hComboBox_Driver_Directories_Files, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0); // удаляем текущее содержимое списка
	WIN32_FIND_DATA FindFileData; // структура для хранения информации и найденном файле/каталоге
	HANDLE hFind; // дескриптор поиска
	std::wstring temp = cur_path_buf;
	temp += L"*.*";
	hFind = FindFirstFile(temp.c_str(), &FindFileData); // определяем первый файл/каталог, содержащийся по текущему пути
	do // перечисляем все файлы/каталоги, содержащиеся по текущему пути
	{
		if (lstrcmp(FindFileData.cFileName, L".") && lstrcmp(FindFileData.cFileName, L".."))
		{
			SendMessage(hComboBox_Driver_Directories_Files, CB_ADDSTRING, (WPARAM)0, (LPARAM)FindFileData.cFileName);
		}
	} while (FindNextFile(hFind, &FindFileData));
	FindClose(hFind); // закрываем дескриптор поиска
}

BOOL is_file(std::wstring& path)
{
	for (int i = path.size() - 1; i >= 0; --i) // начинаем проходить введенный путь с конца
	{
		if (path[i] == L'.') // встретили точку до символа разделителя (создаем файл)
		{
			return TRUE;
		}
		if (path[i] == L'\\') // встретили встретили символ разделить, и до этого не было точки (создаем каталог)
		{
			return FALSE;
		}
	}
	return FALSE;
}

void get_driver_info(std::wstring& disk)
{
	SetWindowText(hStatic_INFO, L""); // очищаем область для вывода новой информации

	std::wstring info_buf; // буффер-строка для хранения информации

	info_buf += L"Тип объекта: диск\n";
	info_buf += L"Имя диска: ";
	info_buf += disk.c_str();
	info_buf += L'\n';
	info_buf += L"Тип диска: ";

	switch (GetDriveType(disk.c_str()))
	{
	case 0:
		info_buf += L"не определен\n";
		break;
	case 1:
		info_buf += L"корневой каталог диска не существует\n";
		break;
	case DRIVE_REMOVABLE:
		info_buf += L"съемный диск\n";
		break;
	case DRIVE_FIXED:
		info_buf += L"фиксированный диск\n";
		break;
	case DRIVE_REMOTE:
		info_buf += L"удаленный(сетевой) диск\n";
		break;
	case DRIVE_CDROM:
		info_buf += L"CD-ROM\n";
		break;
	case DRIVE_RAMDISK:
		info_buf += L"RAM-диск\n";
		break;
	}

	ULARGE_INTEGER FreeBytesAvailableToCaller;
	ULARGE_INTEGER TotalNumberOfBytes;
	ULARGE_INTEGER TotalNumberOfFreeBytes;

	GetDiskFreeSpaceEx(disk.c_str(), &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes);

	info_buf += L"Объем диска в Гбайтах: ";
	TCHAR TotalNumberOfBytes_str[100];
	wsprintf(TotalNumberOfBytes_str, L"%lu", TotalNumberOfBytes.QuadPart / (1024 * 1024 * 1024));
	info_buf += TotalNumberOfBytes_str;
	info_buf += L'\n';
	info_buf += L"Кол-во свободного места на диске в Гбайтах(для текущего пользователя): ";
	TCHAR FreeBytesAvailableToCaller_str[100];
	wsprintf(FreeBytesAvailableToCaller_str, L"%lu", FreeBytesAvailableToCaller.QuadPart / (1024 * 1024 * 1024));
	info_buf += FreeBytesAvailableToCaller_str;
	info_buf += L'\n';
	info_buf += L"Общее кол-во свободного места на диске в Гбайтах: ";
	TCHAR TotalNumberOfFreeBytes_str[100];
	wsprintf(TotalNumberOfFreeBytes_str, L"%lu", TotalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024));
	info_buf += TotalNumberOfFreeBytes_str;
	info_buf += L'\n';

	TCHAR Name[MAX_PATH];
	TCHAR NameFileSystem[MAX_PATH];

	DWORD SerialNumber;
	DWORD TypeFileSystem;
	DWORD SizeTom;

	GetVolumeInformation(disk.c_str(), Name, sizeof(Name), &SerialNumber, &SizeTom, &TypeFileSystem, NameFileSystem, sizeof(NameFileSystem));

	info_buf += L"Имя тома: ";
	info_buf += Name;
	info_buf += L'\n';
	info_buf += L"Серийный номер тома: ";
	TCHAR SerialNumber_str[100];
	wsprintf(SerialNumber_str, L"%lu", SerialNumber);
	info_buf += SerialNumber_str;
	info_buf += L'\n';
	info_buf += L"Имя файловой системы: ";
	info_buf += NameFileSystem;
	info_buf += L'\n';

	SetWindowText(hStatic_INFO, info_buf.c_str()); // помещаем полученную информацию в область на форме

	hFile = CreateFile(L"E:\\OUTPUT_INFO.txt", GENERIC_WRITE, FILE_SHARE_WRITE, 0, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	WriteFile(hFile, (TCHAR*)info_buf.c_str(), info_buf.size() * 2, NULL, NULL); // записываем полученную информацию в специальный файл
	CloseHandle(hFile);
}

void get_dir_file_info(std::wstring& file_dir)
{
	WIN32_FIND_DATA FindFileData; // структура для хранения информации и найденном файле/каталоге
	std::wstring info_buf; // буффер-строка для хранения информации
	
	FindFirstFile(file_dir.c_str(), &FindFileData); // определяем файл/каталог, содержащийся по текущему пути
	
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		info_buf += L"Тип объекта: каталог\n";
	}
	else
	{
		info_buf += L"Тип объекта: файл\n";
	}

	info_buf += L"Имя: ";
	info_buf += FindFileData.cFileName;
	info_buf += L'\n';
	info_buf += L"Альтернативное имя: ";
	info_buf += FindFileData.cAlternateFileName;
	info_buf += L'\n';

	info_buf += L"Атрибуты:";
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
	{
		info_buf += L" |архивный| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
	{
		info_buf += L" |сжатый| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
	{
		info_buf += L" |зашифрованный| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
	{
		info_buf += L" |скрытый| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
	{
		info_buf += L" |отсутствуют|";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
	{
		info_buf += L" |данные доступны не сразу| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
	{
		info_buf += L" |только для чтения| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		info_buf += L" |имеет связную точку монтирования| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
	{
		info_buf += L" |разреженный| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
	{
		info_buf += L" |системный| ";
	}
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
	{
		info_buf += L" |временный| ";
	}
	info_buf += L'\n';

	DWORD byte_size = (FindFileData.nFileSizeHigh * (MAXWORD + 1)) + FindFileData.nFileSizeLow;
	TCHAR byte_size_str[100];
	wsprintf(byte_size_str, L"%u", byte_size);
	info_buf += L"Размер объекта в байтах: ";
	info_buf += byte_size_str;
	info_buf += L'\n';

	SYSTEMTIME creation_time;
	FILETIME creation_file_time = FindFileData.ftCreationTime;
	FileTimeToSystemTime(&creation_file_time, &creation_time);
	SystemTimeToTzSpecificLocalTime(NULL, &creation_time, &creation_time);
	info_buf += L"Дата создания: ";
	TCHAR creation_day_str[5];
	wsprintf(creation_day_str, L"%d", creation_time.wDay);
	if (creation_time.wDay / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += creation_day_str;
	info_buf += L'.';
	TCHAR creation_month_str[5];
	wsprintf(creation_month_str, L"%d", creation_time.wMonth);
	if (creation_time.wMonth / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += creation_month_str;
	info_buf += L'.';
	TCHAR creation_year_str[5];
	wsprintf(creation_year_str, L"%d", creation_time.wYear);
	info_buf += creation_year_str;
	info_buf += L" ";
	TCHAR creation_hour_str[5];
	wsprintf(creation_hour_str, L"%d", creation_time.wHour);
	if (creation_time.wHour / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += creation_hour_str;
	info_buf += L':';
	TCHAR creation_minute_str[5];
	wsprintf(creation_minute_str, L"%d", creation_time.wMinute);
	if (creation_time.wMinute / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += creation_minute_str;
	info_buf += L'\n';

	SYSTEMTIME access_time;
	FILETIME access_file_time = FindFileData.ftLastAccessTime;
	FileTimeToSystemTime(&access_file_time, &access_time);
	SystemTimeToTzSpecificLocalTime(NULL, &access_time, &access_time);
	info_buf += L"Дата последнего обращения: ";
	TCHAR access_day_str[5];
	wsprintf(access_day_str, L"%d", access_time.wDay);
	if (access_time.wDay / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += access_day_str;
	info_buf += L'.';
	TCHAR access_month_str[5];
	wsprintf(access_month_str, L"%d", access_time.wMonth);
	if (access_time.wMonth / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += access_month_str;
	info_buf += L'.';
	TCHAR access_year_str[5];
	wsprintf(access_year_str, L"%d", access_time.wYear);
	info_buf += access_year_str;
	info_buf += L'\n';

	SYSTEMTIME write_time;
	FILETIME write_file_time = FindFileData.ftLastWriteTime;
	FileTimeToSystemTime(&write_file_time, &write_time);
	SystemTimeToTzSpecificLocalTime(NULL, &write_time, &write_time);
	info_buf += L"Дата внесения последних изменений: ";
	TCHAR write_day_str[5];
	wsprintf(write_day_str, L"%d", write_time.wDay);
	if (write_time.wDay / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += write_day_str;
	info_buf += L'.';
	TCHAR write_month_str[5];
	wsprintf(write_month_str, L"%d", write_time.wMonth);
	if (write_time.wMonth / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += write_month_str;
	info_buf += L'.';
	TCHAR write_year_str[5];
	wsprintf(write_year_str, L"%d", write_time.wYear);
	info_buf += write_year_str;
	info_buf += L" ";
	TCHAR write_hour_str[5];
	wsprintf(write_hour_str, L"%d", write_time.wHour);
	if (write_time.wHour / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += write_hour_str;
	info_buf += L':';
	TCHAR write_minute_str[5];
	wsprintf(write_minute_str, L"%d", write_time.wMinute);
	if (write_time.wMinute / 10 == 0)
	{
		info_buf += L'0';
	}
	info_buf += write_minute_str;
	info_buf += L'\n';

	SetWindowText(hStatic_INFO, info_buf.c_str()); // помещаем полученную информацию в область на форме

	hFile = CreateFile(L"E:\\OUTPUT_INFO.txt", GENERIC_WRITE, FILE_SHARE_WRITE, 0, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	WriteFile(hFile, (TCHAR*)info_buf.c_str(), info_buf.size() * 2, NULL, NULL); // записываем полученную информацию в специальный файл
	CloseHandle(hFile);
}