#include <msclr/marshal_cppstd.h>
#include <Sim.h>

public ref class SimManaged
{
public:
	SimManaged(System::String ^xml);
	~SimManaged() { this->!SimManaged(); }
	!SimManaged() { delete simulation_; }

	void Initialize()
	{
		simulation_->Initialize();
	}

	bool Step()
	{
		return simulation_->Step();
	}

	property double Prevalence
	{
		double get()
		{
			return simulation_->GetPrevalence();
		}
	}

	property double Incidence
	{
		double get()
		{
			return simulation_->GetIncidence();
		}
	}

	property System::Collections::Generic::List<System::String ^> ^Messages
	{
		System::Collections::Generic::List<System::String ^> ^get()
		{
			System::Collections::Generic::List<System::String ^> ^messages = gcnew System::Collections::Generic::List<System::String ^>();
			while(!simulation_->GetEventParams()->outputMessageQueue.empty())
			{
				System::String ^message = msclr::interop::marshal_as<System::String ^>(simulation_->GetEventParams()->outputMessageQueue.front());
				simulation_->GetEventParams()->outputMessageQueue.pop_front();
				messages->Add(message);
			}
			return messages;
		}
	}

private:
	Sim *simulation_;
};