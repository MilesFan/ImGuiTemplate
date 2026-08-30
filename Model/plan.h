#pragma once
#ifndef PLAN_H
#define PLAN_H
#include "ulid_struct.hh"
#include "TaskProgress.h"
#include <time.h>
namespace Task
{
	enum Type : uint8_t {
		PlannedProductive = 0,
		UnplannedProductive = 1,
		Offer = 2,
		Research = 3,
		Others = 4
	};
	enum Status : uint8_t {
		NotStarted = 0,
		InProgress = 48,
		OnHold = 96,
		Canceled = 128,
		Completed = 255
	};
	enum Location : uint8_t {
		Office = 0,
		BusinessTrip = 1,
		Home = 2,
		ClientSite = 3,
		Remote = 4,
		Other = 5
	};
	class Progress {
	public:
		ulid::ULID Id = {};
		ulid::ULID PlanId = {};
		ulid::ULID PersonId = {};
		float Hours = 0.0f;
		float Percentage = 0.0f;
		time_t ReportDate = 0;
		Location Location = Location::Office;
	};
	class Plan {
	public:
		ulid::ULID Id = {};
		ulid::ULID PersonId = {};
		std::string Name = {};
		std::vector<time_t> Dates = {};
		float Hours = 0.0f;
		float ProgressPercentage = 0.0f;
		std::vector<Progress> Progresses = {};
		Status Status = Status::NotStarted;
		Plan();
		Plan(std::string Name, std::vector<time_t> Dates) : Name(std::move(Name)), Dates(std::move(Dates)) {}
	};
}
#endif // !PLAN_H
