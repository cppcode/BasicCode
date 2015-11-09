// ioacademy.h
// author : smkang
// aa

#ifndef IOACADEMY_H
#define IOACADEMY_H

#if _MSC_VER > 1000
#pragma  once
#endif

#ifdef UNICODE
#undef _UNICODE
#undef UNICODE
#endif

#include <iostream>
#include <vector>
#include <algorithm>
#include <list>
#include <iterator>
#include <map>
#include <Windows.h>
#include <fstream>
#include <sstream>
using namespace std;

#ifdef _DEBUG
	#define IOTRACE(...) printf(__VA_ARGS__)
#else
	#define IOTRACE(...) 
#endif

namespace io 
{
	namespace __private
	{
		template<typename T, typename ... Types> inline void show_imp(T& c, Types& ... args)
		{
			for (auto& n : c) cout << n << "  ";
			cout << endl;
			show_imp(args...);
		}
		void show_imp() { }
	}	
	namespace container
	{
		template<typename ... Types> void show(Types& ... args) { __private::show_imp(args...); }
	}
	namespace message_loop
	{
		void processMessage()
		{
			MSG msg;
			while (GetMessage(&msg, 0, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		void exitMessage() { PostQuitMessage(0); }
	}
	namespace timer
	{
		typedef void(*IO_TIMER_HANDLER)(int);
		map<int, IO_TIMER_HANDLER> __timer_map;

		int __stdcall _InternalTimerProcedure(HWND hwnd, UINT msg, UINT_PTR id, DWORD)
		{
			if (__timer_map[id])
				__timer_map[id](id);
			return 0;
		}

		int setTimer(int ms, IO_TIMER_HANDLER handler)
		{
			int id = ::SetTimer(0, 0, ms, (TIMERPROC)_InternalTimerProcedure);
			__timer_map[id] = handler;
			return id;
		}

		void killTimer(int id)
		{
			::KillTimer(0, id);
			__timer_map.erase(id);
		}
	}


	namespace module
	{
#ifdef _WIN32
#define IOEXPORT __declspec(dllexport)
#else
#define IOEXPORT 
#endif

		void* loadModule(string path)
		{
			return reinterpret_cast<void*>(LoadLibraryA(path.c_str()));
		}
		void unloadModule(void* p)
		{
			FreeLibrary((HMODULE)p);
		}
		void* getFunctionAddress(void* module, string func)
		{
			return reinterpret_cast<void*>(GetProcAddress((HMODULE)module, func.c_str()));
		}
	}
	namespace file
	{
		typedef int(*PFENUMFILE)(string, void*);

		void enumFiles(string path, string filter, PFENUMFILE f, void* param)
		{
			BOOL b = SetCurrentDirectory(path.c_str());

			if (b == false)
			{
				IOTRACE("[DEBUG]  %s directory does not exit\n", path.c_str());
				return;
			}
			WIN32_FIND_DATA wfd = { 0 };
			HANDLE h = ::FindFirstFile(filter.c_str(), &wfd);
			do
			{
				if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
				{
					if (f(wfd.cFileName, param) == 0) break;
				}
			} while (FindNextFile(h, &wfd));

			FindClose(h);
		}
	}
	namespace basic
	{
		int _nextInt()
		{
			static int n = 0;
			++n;
			return n;
		}
		void setCursor(short x, short y)
		{
			COORD coord = { x, y };
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
		}
		class color
		{
			int _red, _green, _blue;
		public:
			color(int r = 0, int g = 0, int b = 0) : _red(r), _green(g), _blue(b) {}
			inline int red()   const { return _red; }
			inline int green() const { return _green; }
			inline int blue()  const { return _blue; }
			inline static color& redColor()
			{
				static color red(255, 0, 0);
				return red;
			}
			inline static color& greenColor()
			{
				static color green(0, 255, 0);
				return green;
			}
			inline static color& blueColor()
			{
				static color blue(0, 0, 255);
				return blue;
			}
		};
	}

	namespace gui
	{
		typedef int(*IO_MESSAGE_HANDLER)(int, int, int, int);
		IO_MESSAGE_HANDLER user_message_handler = 0;
		map<HWND, IO_MESSAGE_HANDLER> __wndproc_map;

		LRESULT __stdcall _InternalMessageProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
		{
			if (__wndproc_map[hwnd])
				__wndproc_map[hwnd]((int)hwnd, msg, wp, lp);

			if (msg == WM_DESTROY)
			{
				ExitProcess(0);
				//PostQuitMessage(0);
			}
			return DefWindowProc(hwnd, msg, wp, lp);
		}

		//----------------------------------------------------------------------------------------------
		int makeWindow(IO_MESSAGE_HANDLER proc, string title = "S/W Design Pattern(C++)")
		{
			WNDCLASS wc = { 0 };
			wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			wc.hCursor = LoadCursor(0, IDC_ARROW);
			wc.hIcon = LoadIcon(0, IDI_WINLOGO);
			wc.hInstance = (HINSTANCE)GetModuleHandle(0);
			wc.lpfnWndProc = _InternalMessageProcedure;

			ostringstream oss;
			oss << "IO_GUI" << basic::_nextInt();

			string cname = oss.str();
			wc.lpszClassName = cname.c_str();
			RegisterClass(&wc);

			HWND hwnd = CreateWindow(cname.c_str(), title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, 0, 0);

			__wndproc_map[hwnd] = proc;

			ShowWindow(hwnd, SW_SHOW);
			return (int)hwnd;
		}

		void setWindowRect(int handle, int x, int y, int w, int h)
		{
			MoveWindow((HWND)handle, x, y, w, h, TRUE);
		}

		void addChild(int parent_handle, int child_handle)
		{
			SetWindowLong((HWND)child_handle, GWL_STYLE, WS_VISIBLE | WS_BORDER | WS_CHILD);
			SetParent((HWND)child_handle, (HWND)parent_handle);
		}

		void setWindowColor(int handle, const basic::color& color)
		{
			HBRUSH brush = CreateSolidBrush(RGB(color.red(), color.green(), color.blue()));
			HBRUSH old = (HBRUSH)SetClassLong((HWND)handle, GCL_HBRBACKGROUND, (LONG)brush);
			DeleteObject(old);
			InvalidateRect((HWND)handle, 0, TRUE);
		}
	}

	namespace ipc
	{
		typedef int(*IPC_SERVER_HANDLER)(int, int, int);

		IPC_SERVER_HANDLER _proxyServerHandler;

		LRESULT CALLBACK IPCWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg >= WM_USER)
				return _proxyServerHandler(msg - WM_USER, wParam, lParam);

			switch (msg)
			{
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		HWND IPCMakeWindow(string name, int show)
		{
			HINSTANCE hInstance = GetModuleHandle(0);
			WNDCLASS wc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			wc.hCursor = LoadCursor(0, IDC_ARROW);
			wc.hIcon = LoadIcon(0, IDI_APPLICATION);
			wc.hInstance = hInstance;
			wc.lpfnWndProc = IPCWndProc;
			wc.lpszClassName = "IPCServer";
			wc.lpszMenuName = 0;
			wc.style = 0;

			RegisterClass(&wc);

			HWND hwnd = CreateWindowEx(0, "IPCServer", name.c_str(), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);

			ShowWindow(hwnd, show);
			return hwnd;
		}


		void IPCProcessMessage(IPC_SERVER_HANDLER handler)
		{
			_proxyServerHandler = handler;
			MSG msg;
			while (GetMessage(&msg, 0, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		//------------------------------------------------------------------
		void startServer(string name, IPC_SERVER_HANDLER handler, int show = SW_HIDE)
		{
			IOTRACE("[DEBUG] %s Server Start...\n", name.c_str());

			IPCMakeWindow(name, show);
			IPCProcessMessage(handler);
		}

		int findServer(string name)
		{
			HWND hwnd = FindWindow(0, name.c_str());

			if (hwnd == 0)
			{
				IOTRACE("[DEBUG] 실패 : %s Server를 찾을수 없습니다.\n", name.c_str());
				return -1;
			}
			return (int)hwnd;
		}

		int sendServer(int serverid, int code, int param1, int param2)
		{
			return SendMessage((HWND)serverid, code + WM_USER, param1, param2);
		}

		template<typename T, int N> class Queue
		{
			T buff[N];
			int front;
			int rear;
		public:
			Queue() { init(); }
			void init() { front = 0; rear = 0; }

			void push(const T& a)
			{
				buff[front] = a;
				if (++front == N) front = 0;
			}
			T pop()
			{
				int temp = buff[rear];
				if (++rear == N) rear = 0;
				return temp;
			}
		};

		template<typename T, int N> class IPCQueue
		{
			typedef Queue<T, N> QUEUE;

			QUEUE* pQueue;
			HANDLE hMutex;
			HANDLE hSemaphore;
			HANDLE hMap;
		public:
			IPCQueue(string name)
			{
				hMap = CreateFileMappingA((HANDLE)-1, 0, PAGE_READWRITE,0, sizeof(QUEUE), name.c_str());
				pQueue = (QUEUE*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE,0, 0, 0);

				hSemaphore = CreateSemaphoreA(0, 0, N, string(name + "_semaphore").c_str());
				hMutex = CreateMutexA(0, 0, string(name + "_mutex").c_str());

				if (GetLastError() != ERROR_ALREADY_EXISTS)
					pQueue->init();
			}
			void push(const T& a)
			{
				WaitForSingleObject(hMutex, INFINITE);
				pQueue->push(a);
				ReleaseMutex(hMutex);
			}

			T pop()
			{
				WaitForSingleObject(hSemaphore, INFINITE);
				WaitForSingleObject(hMutex, INFINITE);
				T temp = pQueue->pop();
				ReleaseMutex(hMutex);

				return temp;
			}
		};
	}
}

namespace io
{
	using namespace container;
	using namespace timer;
	using namespace message_loop;
	using namespace module;
	using namespace file;
	using namespace basic;
	using namespace gui;
	using namespace ipc;
}

#endif
