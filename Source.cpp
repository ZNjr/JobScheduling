#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "Scheduler.h"
#include "Operation.h"

namespace fs = std::experimental::filesystem;

int main(int argc, const char* argv[]) {
	int k = std::stoi(argv[argc - 1]);
	std::string path = argv[argc - 3];
	fs::create_directory("out");
	int file_counter = 0;
	for (const auto & entry : fs::directory_iterator(path)) {
		std::fstream task_file(entry.path(), std::ios_base::in);
		int number_of_machines;
		int number_of_tasks;
		task_file >> number_of_tasks;
		task_file >> number_of_machines;
		std::vector<std::vector<Operation>> tasks(number_of_tasks);
		for (auto & row : tasks) row = std::vector<Operation>(number_of_machines);
		for (int t = 0; t < number_of_tasks; ++t)
			for (int m = 0; m < number_of_machines; ++m) {
				int machine_number, duration;
				task_file >> machine_number;
				task_file >> duration;
				tasks[t][m] = Operation(duration, machine_number, t);
			}
		Scheduler scheduler = Scheduler(tasks, number_of_tasks, number_of_machines);
		scheduler.k = k;
		scheduler.schedule(SchedulingAlgorithm::ShiftingBottleneck);
		scheduler.save_schedule_to_file("out\\o0" + std::to_string(file_counter));
		task_file.close();
		++file_counter;
	}
	int i;
	std::cin >> i;
	return 0;
};