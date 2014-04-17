#include <msclr/marshal_cppstd.h>
#include "SimManaged.h"

SimManaged::SimManaged(System::String ^xml)
{
	std::string xmlString = msclr::interop::marshal_as<std::string>(xml);
	simulation_ = new Sim(xmlString);
}