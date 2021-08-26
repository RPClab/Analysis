#include "Screen.hpp"

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define VC_EXTRALEAN
  #include <Windows.h>
#elif defined(__linux__)
  #include <sys/ioctl.h>
#endif

#include <iostream>

void Clear()
{
#if defined _WIN32
  std::system("cls");
  //clrscr(); // including header file : conio.h
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
  std::cout<< u8"\033[2J\033[1;1H"; //Using ANSI Escape Sequences
#elif defined (__APPLE__)
  std::system("clear");
#endif
}

void get_terminal_size(int& width, int& height)
{
#if defined(_WIN32)
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  width = static_cast<int>(csbi.dwSize.X);
  height = static_cast<int>(csbi.dwSize.Y);
#elif defined(__linux__)
  struct winsize w;
  ioctl(fileno(stdout), TIOCGWINSZ, &w);
  width = static_cast<int>(w.ws_col);
  height = static_cast<int>(w.ws_row);
#endif
}

void BoxedText(const fmt::text_style &ts, const std::string& message)
{
  static int width{0};
  static int height{0};
  get_terminal_size(width, height);
  fmt::print(ts,
  "┌{0:─^{2}}┐\n"
  "│{1: ^{2}}│\n"
  "└{0:─^{2}}┘\n","", message, width-2);
}

void CenterXText(const fmt::text_style& style, const std::string& text)
{
  static int width{0};
  static int height{0};
  get_terminal_size(width, height);
  fmt::print(style,"{0:^{1}}\n",text,width);
}
