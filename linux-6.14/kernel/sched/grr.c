#include "sched.h"

#ifdef CONFIG_GRR_SCHED

#define GRR_TIME_SLICE_MS 100

extern int grr_cpu_group[NR_CPUS];

void init_grr_rq(struct grr_rq *grr_rq)
{
	INIT_LIST_HEAD(&grr_rq->queue);
	grr_rq->next_balance = jiffies + msecs_to_jiffies(500);
}

static void enqueue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
	struct grr_rq *grr = &rq->grr;

	if (p->grr_time_slice <= 0)
		p->grr_time_slice = msecs_to_jiffies(GRR_TIME_SLICE_MS);

	list_add_tail(&p->grr_list, &grr->queue);
	grr->nr_running++;
	add_nr_running(rq, 1);
}

static bool dequeue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
	struct grr_rq *grr = &rq->grr;

	list_del_init(&p->grr_list);
	grr->nr_running--;
	sub_nr_running(rq, 1);
	return true;
}

static void yield_task_grr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	struct grr_rq *grr = &rq->grr;

	if (grr->nr_running > 1)
		list_move_tail(&p->grr_list, &grr->queue);
}

static void wakeup_preempt_grr(struct rq *rq, struct task_struct *p, int flags)
{
}

static struct task_struct *pick_next_task_grr(struct rq *rq, struct task_struct *prev)
{
	struct grr_rq *grr = &rq->grr;
	struct task_struct *p;

	if (!grr->nr_running)
		return NULL;

	p = list_first_entry(&grr->queue, struct task_struct, grr_list);
	p->se.exec_start = rq_clock_task(rq);
	return p;
}

static void put_prev_task_grr(struct rq *rq, struct task_struct *prev, struct task_struct *next)
{
}

static void set_next_task_grr(struct rq *rq, struct task_struct *p, bool first)
{
	p->se.exec_start = rq_clock_task(rq);
}

#ifdef CONFIG_SMP
static void grr_build_group_cpumask(int group, struct cpumask *mask)
{
	int cpu;

	cpumask_clear(mask);

	if (num_online_cpus() == 1) {
		cpumask_set_cpu(cpumask_first(cpu_online_mask), mask);
		return;
	}

	for_each_online_cpu(cpu) {
		if (READ_ONCE(grr_cpu_group[cpu]) == group)
			cpumask_set_cpu(cpu, mask);
	}
}

static int grr_find_idlest_cpu_in_mask(struct task_struct *p, const struct cpumask *mask)
{
	int cpu, best = -1;
	unsigned int best_nr = UINT_MAX;

	for_each_cpu(cpu, mask) {
		struct rq *rq;

		if (!cpu_online(cpu))
			continue;

		if (!cpumask_test_cpu(cpu, p->cpus_ptr))
			continue;

		rq = cpu_rq(cpu);

		if (rq->nr_running < best_nr) {
			best_nr = rq->nr_running;
			best = cpu;
		}
	}

	return best;
}

int select_task_rq_grr(struct task_struct *p, int cpu, int flags)
{
	int group = READ_ONCE(p->grr_group) ? READ_ONCE(p->grr_group) : GRR_DEFAULT;
	struct cpumask gmask;
	struct cpumask allowed_in_group;
	int best;

	grr_build_group_cpumask(group, &gmask);
	cpumask_and(&allowed_in_group, &gmask, p->cpus_ptr);

	if (!cpumask_empty(&allowed_in_group)) {
		best = grr_find_idlest_cpu_in_mask(p, &allowed_in_group);
		if (best >= 0)
			return best;
	}

	best = grr_find_idlest_cpu_in_mask(p, p->cpus_ptr);
	if (best >= 0)
		return best;

	return cpu;
}
#endif /* CONFIG_SMP */

static void task_tick_grr(struct rq *rq, struct task_struct *p, int queued)
{
	if (p->grr_time_slice > 1) {
		p->grr_time_slice--;
		return;
	}

	p->grr_time_slice = msecs_to_jiffies(GRR_TIME_SLICE_MS);

	if (rq->grr.nr_running > 1) {
		list_move_tail(&p->grr_list, &rq->grr.queue);
		resched_curr(rq);
	}
}

static void switched_to_grr(struct rq *rq, struct task_struct *p)
{
	if (p->on_rq && rq->curr != p)
		resched_curr(rq);

	if (p->grr_time_slice <= 0)
		p->grr_time_slice = msecs_to_jiffies(GRR_TIME_SLICE_MS);
}

static void update_curr_grr(struct rq *rq)
{
}

static void prio_changed_grr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

DEFINE_SCHED_CLASS(grr) = {
	.enqueue_task   = enqueue_task_grr,
	.dequeue_task   = dequeue_task_grr,
	.yield_task     = yield_task_grr,
	.wakeup_preempt = wakeup_preempt_grr,

	.pick_next_task = pick_next_task_grr,
	.put_prev_task  = put_prev_task_grr,
	.set_next_task  = set_next_task_grr,

#ifdef CONFIG_SMP
	.select_task_rq = select_task_rq_grr,
	.balance        = grr_balance,
#endif
	.task_tick      = task_tick_grr,

	.switched_to    = switched_to_grr,
	.prio_changed   = prio_changed_grr,
	.update_curr    = update_curr_grr,
};
#endif
