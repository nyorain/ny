extern int main(int argc, char** argv);

/// This function is needed because from C++ it is not allowed
/// to call the main function. From C it is, so we call this function
/// compiled as a C unit from our C++ code (see android/activity.cpp)
/// instead of main directly. We also compile this file with
/// exception support.
int mainProxyC(int argc, char** argv)
{
	return main(argc, argv);
}
