#pragma once
#include<exception>

struct ImageReadException : public std::exception {
	const char* what() const throw ()
	{
		return "Camera image read failed!";
	}
};

struct CameraException : public std::exception {
	const char* what() const throw ()
	{
		return "Camera address not found!";
	}
};

struct NoDeviceException : public std::exception {
	const char* what() const throw ()
	{
		return "No devices found!";
	}
};

struct InputException : public std::exception {
	const char* what() const throw() {
		return "Input Error!";
	}
};