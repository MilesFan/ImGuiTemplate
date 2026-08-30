#include "plan.h"
namespace Task
{
	Plan::Plan()
	{
		std::srand((unsigned int)std::time(nullptr));
		ulid::EncodeNowRand(Id);
	}
}