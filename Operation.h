#pragma once

class Operation {
public:
	int duration;
	int machine;
    int task;
	int start;
	Operation(int duration, int machine, int task);
	Operation();
};
