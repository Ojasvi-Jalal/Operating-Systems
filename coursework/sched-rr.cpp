/*
 * Round-robin Scheduling Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (1)
 */

/*
 * STUDENT NUMBER: s1612970
 */
#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>
#include <infos/drivers/ata/ata-device.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * A round-robin scheduling algorithm
 */
class RoundRobinScheduler : public SchedulingAlgorithm
{
public:
	/**
	 * Returns the friendly name of the algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "rr"; }

	/**
	 * Called when a scheduling entity becomes eligible for running.
	 * @param entity
	 */
	void add_to_runqueue(SchedulingEntity& entity) override
	{
		//make sure interrupts are disabled when  manipulating the run queue
		UniqueIRQLock l;
		//appends the scheduling entity to the end of the list
		runqueue.enqueue(&entity);
	}

	/**
	 * Called when a scheduling entity is no longer eligible for running.
	 * @param entity
	 */
	void remove_from_runqueue(SchedulingEntity& entity) override
	{
		//make sure interrupts are disabled when  manipulating the run queue
		UniqueIRQLock l;
		//removes the entity from the list containing the current runqueue
		runqueue.remove(&entity);
	}

	/**
	 * Called every time a scheduling event occurs, to cause the next eligible entity
	 * to be chosen.  The next eligible entity might actually be the same entity, if
	 * e.g. its timeslice has not expired.
	 */
	SchedulingEntity *pick_next_entity() override
	{
		//returns null if the list containing the current runqueue is empty. 
		if (runqueue.count() == 0) return NULL;

		//simply return the  element if the current runqueue has only one element.
		else if(runqueue.count()== 1) return runqueue.first();

		/*if the list is not empty then remove the entity from the front of the list and 
		**place it at the back, allow this entity to rn for its timeslice
		*/ 
		else{
			SchedulingEntity* entity = runqueue.dequeue(); //removes the first element and stores it in a variable called entity.
			runqueue.enqueue(entity); //add the removed element to the end of the list
			return entity;
		}

	}

private:
	// A list containing the current runqueue.
	List<SchedulingEntity *> runqueue;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(RoundRobinScheduler);
