#include "Scheduler.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <climits>
#include <random>
#include <ctime>
#include <numeric>
#include <stack>

#include <ilcplex\ilocplex.h>

void Scheduler::allocate_resources()
{
	additional_edges = std::vector<std::vector<OperationIndex>>(number_of_tasks);
	machine_map = std::vector<std::vector<int>>(number_of_tasks);
	time_to_end_task = std::vector<std::vector<int>>(number_of_tasks);
	operation_release_time = std::vector<std::vector<int>>(number_of_tasks);
	for (int t = 0; t < number_of_tasks; t++) {
		additional_edges[t] = std::vector<OperationIndex>(number_of_machines);
		machine_map[t] = std::vector<int>(number_of_machines);
		time_to_end_task[t] = std::vector<int>(number_of_machines);
		operation_release_time[t] = std::vector<int>(number_of_machines);
	}
}

void Scheduler::shifting_bottleneck() {
	result_schedule = std::vector < std::vector<Operation>>((k - 1) * 5 + number_of_machines);
	std::vector<bool> done(number_of_machines);
	int c_max = calc_basic_values();
	for (int i = 0; i < number_of_machines; ++i) {
		int max_delay_machine = 0;
		int max_delay = INT_MIN;
		std::vector<std::vector<Operation>> schedule;
		for (int j = 0; j < number_of_machines; ++j) {
			calc_constrains(c_max, j);
			if (!done[j]) {
				std::vector<std::vector<Operation>> current_schedule;
				int current_min_delay = minimize_delay(j, current_schedule);
				if (current_min_delay > max_delay) {
					max_delay = current_min_delay;
					schedule = std::move(current_schedule);
					max_delay_machine = j;
				}
			}
		}
		int machine_index = (max_delay_machine <= 4) ? max_delay_machine * k : (k - 1) * 5 + max_delay_machine;
		//min_delay_schedule();
		/*for (const auto machine : schedule) {
			for (const auto op : machine)
				std::cout << op.task << " ";
			std::cout << std::endl;
		}*/
		add_edges(schedule);
		for (int m = 0; m < schedule.size(); ++m)
			result_schedule[machine_index+m] = std::move(schedule[m]);
		c_max += max_delay;
		done[max_delay_machine] = true;
	}
}

int Scheduler::calc_basic_values() {
	int c_max = 0;
	for (int t = 0; t < number_of_tasks; ++t) {
		int c_i = 0;
		for (int m = 0; m < number_of_machines; ++m) {
			operation_release_time[t][m] = c_i;
			c_i += tasks[t][m].duration;
			machine_map[t][tasks[t][m].machine] = m;
		}
		c_max = (c_i > c_max) ? c_i : c_max;
		for (int m = 0; m < number_of_machines; ++m)
			time_to_end_task[t][m] = c_i - operation_release_time[t][m];
	}
	return c_max;
}

int Scheduler::minimize_delay(int machine, std::vector<std::vector<Operation>> &min_delay_schedule) {
	std::vector<int> permutation(number_of_tasks);
	std::vector<std::vector<int>> population(POPULATION_SIZE);
	std::vector<int> delays(POPULATION_SIZE);
	std::vector<std::vector<Operation>> current_schedule;
	int min_deley = INT_MAX;
	int generation = GENERATIONS;
	bool improved = false;
	std::srand(std::time(NULL));
	for (int i = 0; i < permutation.size(); ++i) permutation[i] = i;
	for (int i = 0;i<population.size();++i) {
		std::random_shuffle(permutation.begin(), permutation.end());
		population[i] = permutation;
		single_machine_schedule_from_permutation(population[i], machine, current_schedule);
		delays[i] = max_delay(current_schedule);
	}
	distribution_weights(delays);
	do {
		improved = false;	
		int current_max_delay = evol(machine, population, current_schedule, delays);
		if (current_max_delay < min_deley) {
			min_deley = current_max_delay;
			min_delay_schedule = std::move(current_schedule);
			improved = true;
		}
	} while (improved || generation-- < 0);
	return min_deley;
}

int Scheduler::evol(int machine, std::vector<std::vector<int>> &population, std::vector<std::vector<Operation>> &schedule, std::vector<int>& weights) {
	std::vector<std::vector<int>> new_population(population.size());
	std::random_device rd;
	std::mt19937 generator(rd());
	std::discrete_distribution<int> distribution(weights.begin(),weights.end());
	for (int i = 0; i < population.size(); ++i) {
		if ((rand() % 100) < CROSSOVER_PROBABILITY) 
			crossover(population[distribution(generator)], population[distribution(generator)], new_population[i]);
		else new_population[i] = population[distribution(generator)];
		if ((rand() % 100) < MUTATION_RATE)
			mutation(new_population[i]);
	}
	int delay = population_delay(machine, new_population, schedule, weights);
	population = std::move(new_population);
	return delay;
}

void Scheduler::crossover(std::vector<int>& parent_x, std::vector<int> &parent_y, std::vector<int> &child) {
	child = std::vector<int>(parent_x.size());
	std::vector<int> subset(parent_x);
	std::vector<bool> deleted(parent_x.size());
	std::vector<bool> selected(parent_x.size());
	std::random_shuffle(subset.begin(), subset.end());
	int subset_size = std::ceil(((double)(subset.size()*CROSSOVER_RATE)) / 100.0);
	for (int i = 0; i < subset_size; ++i) {
		selected[subset[i]] = true;
		deleted[std::distance(parent_y.begin(), std::find(parent_y.begin(), parent_y.end(), parent_x[subset[i]]))] = true;
		child[subset[i]] = parent_x[subset[i]];
	}
	int j = 0;
	for (int i = 0; i < child.size(); ++i) {
		if (!selected[i]) {
			while (deleted[j++]);
			child[i] = parent_y[j-1];
		}
	}
}

void Scheduler::mutation(std::vector<int>& chromosome) {
	std::swap(chromosome[rand() % chromosome.size()], chromosome[rand() % chromosome.size()]);
}

void Scheduler::distribution_weights(std::vector<int>& delays) {
	int max_delay = *std::max_element(delays.begin(), delays.end());
	double average = std::accumulate(delays.begin(), delays.end(), 0.0) / delays.size();
	for (auto& delay : delays) {
		delay = max_delay + 1 - delay;
		delay *= std::ceil(std::pow((double)delay / ((average!=0)?average:1), 2));
	}
}

int Scheduler::population_delay(int machine, std::vector<std::vector<int>> &population, std::vector<std::vector<Operation>>& schedule, std::vector<int>& weights)
{
	int min_delay = INT_MAX;
	std::vector<std::vector<Operation>> current_schedule ;
	for (int i = 0; i < population.size(); ++i) {
		single_machine_schedule_from_permutation(population[i], machine, current_schedule);
		weights[i] = max_delay(current_schedule);
		if (weights[i] < min_delay) {
			min_delay = weights[i];
			schedule = std::move(current_schedule);
		}
	}
	distribution_weights(weights);
	return min_delay;
}

int Scheduler::max_delay(std::vector<std::vector<Operation>>& schedule) {
	int max_delay = INT_MIN;
	for (const auto& machine : schedule)
		for (const auto& operation : machine) {
			int current_delay = operation.start + operation.duration - constrains[operation.task][2];
			max_delay = (current_delay > max_delay) ? current_delay : max_delay;
		}
	return max_delay;
}

void Scheduler::single_machine_schedule_from_permutation(std::vector<int>& permutation, int machine, std::vector<std::vector<Operation>>& schedule){
	schedule = std::vector<std::vector<Operation>>((machine <= 4) ? k : 1);
	for (auto &machine_schedule : schedule) {
		machine_schedule = std::vector<Operation>();
		machine_schedule.reserve(number_of_tasks);
	}
	for (int i = 0; i < permutation.size(); ++i) {
		std::vector<Operation>::iterator current_iter;
		std::vector<Operation>::iterator iter;
		Operation operation(constrains[permutation[i]][0], machine, permutation[i]);
		operation.start = machine_free_time(schedule[0], operation.duration, constrains[permutation[i]][1],iter);
		int current_machine = 0;
		if(machine<=4)
			for (int m = 1; m < k; ++m) {
				int current_machine_free_time = machine_free_time(schedule[m], operation.duration, constrains[permutation[i]][1],current_iter);
				if (current_machine_free_time < operation.start) {
					operation.start = current_machine_free_time;
					current_machine = m;
					iter = current_iter;
				}
			}
		schedule[current_machine].insert(iter, operation);
	}
}

void Scheduler::calc_constrains(int c_max, int machine) {
	constrains = std::vector<std::vector<int>>(number_of_tasks);
	for (auto& constrain : constrains) constrain = std::vector<int>(3);
	for (int t = 0; t < tasks.size(); ++t) {
		constrains[t][0] = tasks[t][machine_map[t][machine]].duration;
		constrains[t][1] = operation_release_time[t][machine_map[t][machine]];
		constrains[t][2] = c_max - time_to_end_task[t][machine_map[t][machine]] + constrains[t][0];
	}
}

void Scheduler::add_edges(std::vector<std::vector<Operation>>& schedule) {
	for (int m = 0; m < schedule.size(); ++m) 
		for (int o = 0; o < schedule[m].size() - 1; ++o) {
			try {
				Operation prev_operation = schedule.at(m).at(o);
				Operation next_operation = schedule[m][o + 1];
				add_edge(prev_operation.task, machine_map[prev_operation.task][prev_operation.machine],next_operation.task, machine_map[next_operation.task][next_operation.machine]);
			}
			catch (const std::out_of_range& oor) {
				break;
			}
		}
}

void Scheduler::add_edge(int start_task, int start_operation,int end_task, int end_operation) {
	additional_edges[start_task][start_operation].add(end_task, end_operation, OperationIndex::Type::Forward);
	additional_edges[end_task][end_operation].add(start_task, start_operation, OperationIndex::Type::Backward);
	if (operation_release_time[start_task][start_operation] + tasks[start_task][start_operation].duration > operation_release_time[end_task][end_operation]) {
		update_release_time(end_task, end_operation);
	}
	if (time_to_end_task[end_task][end_operation] + tasks[end_task][end_operation].duration > time_to_end_task[start_task][start_operation])
		update_time_to_end(start_task, start_operation);
}

void Scheduler::update_release_time(int task, int operation) {
	std::stack<int> stack = std::stack<int>();
	stack.push(task);
	stack.push(operation);
	OperationIndex backward = additional_edges[task][operation];
	operation_release_time[task][operation] = operation_release_time[backward.backward_task][backward.backward_operation] + tasks[backward.backward_task][backward.backward_operation].duration;
	do {
		operation = stack.top();
		stack.pop();
		task = stack.top();
		stack.pop();
		for (int i = operation; i < number_of_machines; ++i) {
			if (i != operation) {
				if (operation_release_time[task][i] < operation_release_time[task][i - 1] + tasks[task][i - 1].duration) {
					operation_release_time[task][i] = operation_release_time[task][i - 1] + tasks[task][i - 1].duration;
				}
				else break;
			}
			if (additional_edges[task][i].forward())
				if (operation_release_time[task][i] + tasks[task][i].duration > operation_release_time[additional_edges[task][i].forward_task][additional_edges[task][i].forward_operation]) {
					stack.push(additional_edges[task][i].forward_task);
					stack.push(additional_edges[task][i].forward_operation);
					operation_release_time[additional_edges[task][i].forward_task][additional_edges[task][i].forward_operation] = operation_release_time[task][i] + tasks[task][i].duration;
				}
		}
	} while (!stack.empty());
}

void Scheduler::update_time_to_end(int task, int operation) {
	OperationIndex forward = additional_edges[task][operation];
	time_to_end_task[task][operation] = time_to_end_task[forward.forward_task][forward.forward_operation] + tasks[forward.forward_task][forward.forward_operation].duration;
	for (int i = operation; i >= 0; --i) {
		if (i != operation)
			if (time_to_end_task[task][i] < time_to_end_task[task][i + 1] + tasks[task][i + 1].duration)
				time_to_end_task[task][i] = time_to_end_task[task][i + 1] + tasks[task][i + 1].duration;
		if (additional_edges[task][i].backward())
			if (time_to_end_task[task][i] + tasks[task][i].duration > time_to_end_task[additional_edges[task][i].backward_task][additional_edges[task][i].backward_operation])
				update_release_time(additional_edges[task][i].backward_task, additional_edges[task][i].backward_operation);

	}
}

std::vector<int> Scheduler::min_delay_schedule()
{
	IloEnv env;
	IloModel model(env);
	IloCplex cplex(model);
	IloNumVarArray vars(env);

	cplex.setParam(IloCplex::RootAlg, IloCplex::Barrier);

	vars.add(IloNumVar(env));
	model.add(IloMinimize(env, vars[0]));

	for (int t = 0; t < number_of_tasks; ++t) {
		vars.add(IloNumVar(env));
		model.add(vars[0] >= vars[t + 1] + constrains[t][0] - constrains[t][2]);
		model.add(vars[t + 1] >= constrains[t][1]);
	}
	for (int i = 0; i < number_of_tasks - 1; ++i)
		for (int j = i + 1; j < number_of_tasks; ++j) {
			model.add((vars[i + 1] - vars[j + 1] >= constrains[j][0]) || (vars[j + 1] - vars[i + 1] >= constrains[i][0]));
		}
	cplex.solve();
	if (cplex.getStatus() == IloAlgorithm::Optimal) {
		IloNumArray vals(env);
		cplex.getValues(vals, vars);
		env.out() << "values = " << vals << std::endl;
	}
	return std::vector<int>();
}

//void Scheduler::calc_constrains(int c_max, int machine) {
//	constrains = std::vector<std::vector<int>>(number_of_tasks);
//	for (auto& constrain : constrains) constrain = std::vector<int>(3);
//	for (int t = 0; t < tasks.size(); ++t) {
//		for (const auto &operation : tasks[t])
//			if (operation.machine == machine) {
//				constrains[t][0] = operation.duration;
//				break;
//			}
//		set_min_release_time(t, 0, machine, 0);
//		constrains[t][2] = c_max - max_operation_end_time(t, machine_map[t][machine]) + tasks[t][machine_map[t][machine]].duration;
//	}
//}

//void Scheduler::set_min_release_time(int task, int operation, int machine, int current_duration) {
//	if (tasks[task][operation].machine == machine) {
//		if (current_duration > constrains[task][1]) constrains[task][1] = current_duration;
//	}
//	else if (operation < number_of_machines - 1) {
//		set_min_release_time(task, operation + 1, machine, current_duration + tasks[task][operation].duration);
//		if (additional_edges[task][operation].set())
//			set_min_release_time(additional_edges[task][operation].task, additional_edges[task][operation].operation, machine, current_duration + tasks[task][operation].duration);
//	}
//}
//
//int Scheduler::max_operation_end_time(int task, int operation) {
//	if (operation >= number_of_machines - 1) return tasks[task][operation].duration;
//	int additional_edge_duration = (additional_edges[task][operation].set()) ? max_operation_end_time(additional_edges[task][operation].task, additional_edges[task][operation].operation) : 0;
//	return std::max(additional_edge_duration, max_operation_end_time(task, operation + 1)) + tasks[task][operation].duration;
//}