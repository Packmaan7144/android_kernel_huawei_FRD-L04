/*
 *  linux/drivers/thermal/cpu_cooling.c
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/opp.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/topology.h>
#include <trace/events/thermal.h>
#include <trace/events/thermal_power_allocator.h>

/**
 * struct power_table - frequency to power conversion
 * @frequency:	frequency in KHz
 * @power:	power in mW
 *
 * This structure is built when the cooling device registers and helps
 * in translating frequency to power and viceversa.
 */
struct power_table {
	u32 frequency;
	u32 power;
};

/**
 * struct cpufreq_cooling_device - data for cooling device with cpufreq
 * @id: unique integer value corresponding to each cpufreq_cooling_device
 *	registered.
 * @cool_dev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @cpufreq_state: integer value representing the current state of cpufreq
 *	cooling	devices.
 * @cpufreq_val: integer value representing the absolute value of the clipped
 *	frequency.
 * @allowed_cpus: all the cpus involved for this cpufreq_cooling_device.
 * @last_load: load measured by the latest call to cpufreq_get_actual_power()
 * @time_in_idle: previous reading of the absolute time that this cpu was idle
 * @time_in_idle_timestamp: wall time of the last invocation of
 *	get_cpu_idle_time_us()
 * @dyn_power_table: array of struct power_table for frequency to power
 *	conversion, sorted in ascending order.
 * @dyn_power_table_entries: number of entries in the @dyn_power_table array
 * @cpu_dev: the first cpu_device from @allowed_cpus that has OPPs registered
 * @plat_get_static_power: callback to calculate the static power
 *
 * This structure is required for keeping information of each
 * cpufreq_cooling_device registered. In order to prevent corruption of this a
 * mutex lock cooling_cpufreq_lock is used.
 */
struct cpufreq_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	unsigned int cpufreq_state;
	unsigned int cpufreq_val;
	struct cpumask allowed_cpus;
	u32 last_load;
	u64 *time_in_idle;
	u64 *time_in_idle_timestamp;
	struct power_table *dyn_power_table;
	int dyn_power_table_entries;
	struct device *cpu_dev;
	get_static_t plat_get_static_power;
};
static DEFINE_IDR(cpufreq_idr);
static DEFINE_MUTEX(cooling_cpufreq_lock);

static unsigned int cpufreq_dev_count;

/* notify_table passes value to the CPUFREQ_ADJUST callback function. */
#define NOTIFY_INVALID NULL
static struct cpufreq_cooling_device *notify_device;

/**
 * get_idr - function to get a unique id.
 * @idr: struct idr * handle used to create a id.
 * @id: int * value generated by this function.
 *
 * This function will populate @id with an unique
 * id, using the idr API.
 *
 * Return: 0 on success, an error code on failure.
 */
static int get_idr(struct idr *idr, int *id)
{
	int ret;

	mutex_lock(&cooling_cpufreq_lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);
	mutex_unlock(&cooling_cpufreq_lock);
	if (unlikely(ret < 0))
		return ret;
	*id = ret;

	return 0;
}

/**
 * release_idr - function to free the unique id.
 * @idr: struct idr * handle used for creating the id.
 * @id: int value representing the unique id.
 */
static void release_idr(struct idr *idr, int id)
{
	mutex_lock(&cooling_cpufreq_lock);
	idr_remove(idr, id);
	mutex_unlock(&cooling_cpufreq_lock);
}

/* Below code defines functions to be used for cpufreq as cooling device */

/**
 * is_cpufreq_valid - function to check frequency transitioning capability.
 * @cpu: cpu for which check is needed.
 *
 * This function will check the current state of the system if
 * it is capable of changing the frequency for a given @cpu.
 *
 * Return: 0 if the system is not currently capable of changing
 * the frequency of given cpu. !0 in case the frequency is changeable.
 */
static int is_cpufreq_valid(int cpu)
{
	struct cpufreq_policy policy;

	return !cpufreq_get_policy(&policy, cpu);
}

enum cpufreq_cooling_property {
	GET_LEVEL,
	GET_FREQ,
	GET_MAXL,
};

/**
 * get_property - fetch a property of interest for a give cpu.
 * @cpu: cpu for which the property is required
 * @input: query parameter
 * @output: query return
 * @property: type of query (frequency, level, max level)
 *
 * This is the common function to
 * 1. get maximum cpu cooling states
 * 2. translate frequency to cooling state
 * 3. translate cooling state to frequency
 * Note that the code may be not in good shape
 * but it is written in this way in order to:
 * a) reduce duplicate code as most of the code can be shared.
 * b) make sure the logic is consistent when translating between
 *    cooling states and frequencies.
 *
 * Return: 0 on success, -EINVAL when invalid parameters are passed.
 */
static int get_property(unsigned int cpu, unsigned long input,
			unsigned int *output,
			enum cpufreq_cooling_property property)
{
	int i, j;
	unsigned long max_level = 0, level = 0;
	unsigned int freq = CPUFREQ_ENTRY_INVALID;
	int descend = -1;
	struct cpufreq_frequency_table *table =
					cpufreq_frequency_get_table(cpu);

	if (!output)
		return -EINVAL;

	if (!table)
		return -EINVAL;

	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		/* ignore invalid entries */
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		/* ignore duplicate entry */
		if (freq == table[i].frequency)
			continue;

		/* get the frequency order */
		if (freq != CPUFREQ_ENTRY_INVALID && descend == -1)
			descend = !!(freq > table[i].frequency);

		freq = table[i].frequency;
		max_level++;
	}

	/* No valid cpu frequency entry */
	if (max_level == 0)
		return -EINVAL;

	/* max_level is an index, not a counter */
	max_level--;

	/* get max level */
	if (property == GET_MAXL) {
		*output = (unsigned int)max_level;
		return 0;
	}

	if (property == GET_FREQ)
		level = descend ? input : (max_level - input);

	for (i = 0, j = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		/* ignore invalid entry */
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		/* ignore duplicate entry */
		if (freq == table[i].frequency)
			continue;

		/* now we have a valid frequency entry */
		freq = table[i].frequency;

		if (property == GET_LEVEL && (unsigned int)input == freq) {
			/* get level by frequency */
			*output = descend ? j : (max_level - j);
			return 0;
		}
		if (property == GET_FREQ && level == j) {
			/* get frequency by level */
			*output = freq;
			return 0;
		}
		j++;
	}

	return -EINVAL;
}

/**
 * cpufreq_cooling_get_level - for a give cpu, return the cooling level.
 * @cpu: cpu for which the level is required
 * @freq: the frequency of interest
 *
 * This function will match the cooling level corresponding to the
 * requested @freq and return it.
 *
 * Return: The matched cooling level on success or THERMAL_CSTATE_INVALID
 * otherwise.
 */
unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq)
{
	unsigned int val;

	if (get_property(cpu, (unsigned long)freq, &val, GET_LEVEL))
		return THERMAL_CSTATE_INVALID;

	return (unsigned long)val;
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_get_level);

/**
 * get_cpu_frequency - get the absolute value of frequency from level.
 * @cpu: cpu for which frequency is fetched.
 * @level: cooling level
 *
 * This function matches cooling level with frequency. Based on a cooling level
 * of frequency, equals cooling state of cpu cooling device, it will return
 * the corresponding frequency.
 *	e.g level=0 --> 1st MAX FREQ, level=1 ---> 2nd MAX FREQ, .... etc
 *
 * Return: 0 on error, the corresponding frequency otherwise.
 */
static unsigned int get_cpu_frequency(unsigned int cpu, unsigned long level)
{
	int ret = 0;
	unsigned int freq;

	ret = get_property(cpu, level, &freq, GET_FREQ);
	if (ret)
		return 0;

	return freq;
}

/**
 * cpufreq_apply_cooling - function to apply frequency clipping.
 * @cpufreq_device: cpufreq_cooling_device pointer containing frequency
 *	clipping data.
 * @cooling_state: value of the cooling state.
 *
 * Function used to make sure the cpufreq layer is aware of current thermal
 * limits. The limits are applied by updating the cpufreq policy.
 *
 * Return: 0 on success, an error code otherwise (-EINVAL in case wrong
 * cooling state).
 */

extern unsigned int g_ipa_freq_limit[];
static int cpufreq_apply_cooling(struct cpufreq_cooling_device *cpufreq_device,
				 unsigned long cooling_state)
{
	unsigned int cpuid, clip_freq,cur_cluster;
	struct cpumask *mask = &cpufreq_device->allowed_cpus;
	unsigned int cpu = cpumask_any(mask);
	cur_cluster = topology_physical_package_id(cpu);

	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_device->cpufreq_state == cooling_state)
		return 0;

	clip_freq = get_cpu_frequency(cpu, cooling_state);
	if (!clip_freq)
		return -EINVAL;

	cpufreq_device->cpufreq_state = cooling_state;
	cpufreq_device->cpufreq_val = clip_freq;
	notify_device = cpufreq_device;
	g_ipa_freq_limit[cur_cluster] = clip_freq;

	trace_IPA_actor_cpu_cooling(&cpufreq_device->allowed_cpus, cpufreq_device->cpufreq_val,
					cooling_state);
	for_each_cpu(cpuid, mask) {
		if (is_cpufreq_valid(cpuid))
			cpufreq_update_policy(cpuid);
	}

	notify_device = NOTIFY_INVALID;

	return 0;
}

/**
 * cpufreq_thermal_notifier - notifier callback for cpufreq policy change.
 * @nb:	struct notifier_block * with callback info.
 * @event: value showing cpufreq event for which this function invoked.
 * @data: callback-specific data
 *
 * Callback to highjack the notification on cpufreq policy transition.
 * Every time there is a change in policy, we will intercept and
 * update the cpufreq policy with thermal constraints.
 *
 * Return: 0 (success)
 */
static int cpufreq_thermal_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned long max_freq = 0;

	switch (event) {

	case CPUFREQ_ADJUST:
		if (notify_device == NOTIFY_INVALID)
			return NOTIFY_DONE;

		if (cpumask_test_cpu(policy->cpu, &notify_device->allowed_cpus))
			max_freq = notify_device->cpufreq_val;
		else
			return NOTIFY_DONE;

		/* Never exceed user_policy.max */
		if (max_freq > policy->user_policy.max)
			max_freq = policy->user_policy.max;

		if (policy->max != max_freq)
			cpufreq_verify_within_limits(policy, 0, max_freq);

		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

/**
 * build_dyn_power_table() - create a dynamic power to frequency table
 * @cpufreq_device:	the cpufreq cooling device in which to store the table
 * @capacitance: dynamic power coefficient for these cpus
 *
 * Build a dynamic power to frequency table for this cpu and store it
 * in @cpufreq_device.  This table will be used in cpu_power_to_freq() and
 * cpu_freq_to_power() to convert between power and frequency
 * efficiently.  Power is stored in mW, frequency in KHz.  The
 * resulting table is in ascending order.
 *
 * Return: 0 on success, -E* on error.
 */
static int build_dyn_power_table(struct cpufreq_cooling_device *cpufreq_device,
				 u32 capacitance)
{
	struct power_table *power_table;
	struct opp *opp;
	struct device *dev = NULL;
	int num_opps = 0, cpu, i, ret = 0;
	unsigned long freq;
	int nr_cpus;

	rcu_read_lock();

	for_each_cpu(cpu, &cpufreq_device->allowed_cpus) {
		dev = get_cpu_device(cpu);
		if (!dev) {
			dev_warn(&cpufreq_device->cool_dev->device,
				 "No cpu device for cpu %d\n", cpu);
			continue;
		}

		num_opps = opp_get_opp_count(dev);
		if (num_opps > 0) {
			break;
		} else if (num_opps < 0) {
			ret = num_opps;
			goto unlock;
		}
	}

	if (num_opps == 0) {
		ret = -EINVAL;
		goto unlock;
	}

	power_table = kcalloc(num_opps, sizeof(*power_table), GFP_ATOMIC);
	pr_debug("IPA:CPU(85C) POWER TABLE\n			FREQ(MHz)  @ VOLT(mV) :  DYN(mW)+STATIC(mW) = POWER(mW)\n");

	for (freq = 0, i = 0;
	     opp = opp_find_freq_ceil(dev, &freq), !IS_ERR(opp);
	     freq++, i++) {
		u32 freq_mhz, voltage_mv;
		u64 power;
		u32 static_power;
		freq_mhz = freq / 1000000;
		voltage_mv = opp_get_voltage(opp) / 1000;


		/*
		 * Do the multiplication with MHz and millivolt so as
		 * to not overflow.
		 */
		power = (u64)capacitance * freq_mhz * voltage_mv * voltage_mv;
		do_div(power, 1000000000);

		/* frequency is stored in power_table in KHz */
		power_table[i].frequency = freq / 1000;

		/* power is stored in mW */
		power_table[i].power = power;

		nr_cpus = cpumask_weight(&cpufreq_device->allowed_cpus);
		if (0 == nr_cpus) {
			nr_cpus = 1;
		}

		static_power = static_power/nr_cpus;

		(void)cpufreq_device->plat_get_static_power(&cpufreq_device->allowed_cpus,0,voltage_mv*1000,&static_power);
		pr_debug("  %u MHz @ %u mV :  %u + %u = %u mW\n",
		freq_mhz, voltage_mv, power_table[i].power,static_power,power_table[i].power+static_power);


	}

	if (i == 0) {
		ret = PTR_ERR(opp);
		kfree(power_table);
		goto unlock;
	}

	cpufreq_device->cpu_dev = dev;
	cpufreq_device->dyn_power_table = power_table;
	cpufreq_device->dyn_power_table_entries = i;

unlock:
	rcu_read_unlock();
	return ret;
}

static u32 cpu_freq_to_power(struct cpufreq_cooling_device *cpufreq_device,
			     u32 freq)
{
	int i;
	struct power_table *pt = cpufreq_device->dyn_power_table;

	for (i = 1; i < cpufreq_device->dyn_power_table_entries; i++)
		if (freq < pt[i].frequency)
			break;

	return pt[i - 1].power;
}

static u32 cpu_power_to_freq(struct cpufreq_cooling_device *cpufreq_device,
			     u32 power)
{
	int i;
	struct power_table *pt = cpufreq_device->dyn_power_table;

	for (i = 1; i < cpufreq_device->dyn_power_table_entries; i++)
		if (power < pt[i].power)
			break;

	return pt[i - 1].frequency;
}

/**
 * get_load() - get load for a cpu since last updated
 * @cpufreq_device:	&struct cpufreq_cooling_device for this cpu
 * @cpu:	cpu number
 *
 * Return: The average load of cpu @cpu in percentage since this
 * function was last called.
 */
static u32 get_load(struct cpufreq_cooling_device *cpufreq_device, int cpu)
{
	u32 load;
	u64 now, now_idle, delta_time, delta_idle;

	now_idle = get_cpu_idle_time(cpu, &now, 0);
	delta_idle = now_idle - cpufreq_device->time_in_idle[cpu];
	delta_time = now - cpufreq_device->time_in_idle_timestamp[cpu];

	if (delta_time <= delta_idle)
		load = 0;
	else
		load = div64_u64(100 * (delta_time - delta_idle), delta_time);

	cpufreq_device->time_in_idle[cpu] = now_idle;
	cpufreq_device->time_in_idle_timestamp[cpu] = now;

	return load;
}

/**
 * get_static_power() - calculate the static power consumed by the cpus
 * @cpufreq_device:	struct &cpufreq_cooling_device for this cpu cdev
 * @tz:		thermal zone device in which we're operating
 * @freq:	frequency in KHz
 * @power:	pointer in which to store the calculated static power
 *
 * Calculate the static power consumed by the cpus described by
 * @cpu_actor running at frequency @freq.  This function relies on a
 * platform specific function that should have been provided when the
 * actor was registered.  If it wasn't, the static power is assumed to
 * be negligible.  The calculated static power is stored in @power.
 *
 * Return: 0 on success, -E* on failure.
 */
static int get_static_power(struct cpufreq_cooling_device *cpufreq_device,
			    struct thermal_zone_device *tz, unsigned long freq,
			    u32 *power)
{
	struct opp *opp;
	unsigned long voltage;
	struct cpumask *cpumask = &cpufreq_device->allowed_cpus;
	unsigned long freq_hz = freq * 1000;

	if (!cpufreq_device->plat_get_static_power ||
	    !cpufreq_device->cpu_dev) {
		*power = 0;
		return 0;
	}

	rcu_read_lock();

	opp = opp_find_freq_exact(cpufreq_device->cpu_dev, freq_hz,
				  true);
	voltage = opp_get_voltage(opp);

	rcu_read_unlock();

	if (voltage == 0) {
		dev_warn_ratelimited(cpufreq_device->cpu_dev,
				     "Failed to get voltage for frequency %lu: %ld\n",
				     freq_hz, IS_ERR(opp) ? PTR_ERR(opp) : 0);
		return -EINVAL;
	}

	return cpufreq_device->plat_get_static_power(cpumask, tz->passive_delay,
						     voltage, power);
}

/**
 * get_dynamic_power() - calculate the dynamic power
 * @cpufreq_device:	&cpufreq_cooling_device for this cdev
 * @freq:	current frequency
 *
 * Return: the dynamic power consumed by the cpus described by
 * @cpufreq_device.
 */
static u32 get_dynamic_power(struct cpufreq_cooling_device *cpufreq_device,
			     unsigned long freq)
{
	u32 raw_cpu_power;

	raw_cpu_power = cpu_freq_to_power(cpufreq_device, freq);
	return (raw_cpu_power * cpufreq_device->last_load) / 100;
}

/* cpufreq cooling device callback functions are defined below */

/**
 * cpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	struct cpumask *mask = &cpufreq_device->allowed_cpus;
	unsigned int cpu;
	unsigned int count = 0;
	int ret;

	cpu = cpumask_any(mask);

	ret = get_property(cpu, 0, &count, GET_MAXL);

	if (count > 0)
		*state = count;

	return ret;
}

/**
 * cpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	*state = cpufreq_device->cpufreq_state;

	return 0;
}

/**
 * cpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	return cpufreq_apply_cooling(cpufreq_device, state);
}

/**
 * cpufreq_get_requested_power() - get the current power
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	pointer in which to store the resulting power
 *
 * Calculate the current power consumption of the cpus in milliwatts
 * and store it in @power.  This function should actually calculate
 * the requested power, but it's hard to get the frequency that
 * cpufreq would have assigned if there were no thermal limits.
 * Instead, we calculate the current power on the assumption that the
 * immediate future will look like the immediate past.
 *
 * We use the current frequency and the average load since this
 * function was last called.  In reality, there could have been
 * multiple opps since this function was last called and that affects
 * the load calculation.  While it's not perfectly accurate, this
 * simplification is good enough and works.  REVISIT this, as more
 * complex code may be needed if experiments show that it's not
 * accurate enough.
 *
 * Return: 0 on success, -E* if getting the static power failed.
 */
static int cpufreq_get_requested_power(struct thermal_cooling_device *cdev,
				       struct thermal_zone_device *tz,
				       u32 *power)
{
	unsigned long freq = 0;
	int i = 0, cpu = 0, ret = 0;
	u32 static_power = 0, dynamic_power = 0, total_load = 0;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;
	u32 *load_cpu = NULL;

	cpu = cpumask_any_and(&cpufreq_device->allowed_cpus, cpu_online_mask);

	/* None of allowed_cpus are online */
	if (cpu >= nr_cpu_ids) {
		*power = 0;
		return -ENODEV;
	}

	freq = cpufreq_quick_get(cpu);
/*
	if (trace_thermal_power_cpu_get_power_enabled()) {
*/
	if (1) {
		u32 ncpus = cpumask_weight(&cpufreq_device->allowed_cpus);

		load_cpu = devm_kcalloc(&cdev->device, ncpus, sizeof(*load_cpu),
					GFP_KERNEL);
	}

	for_each_cpu(cpu, &cpufreq_device->allowed_cpus) {
		u32 load;

		if (cpu_online(cpu))
			load = get_load(cpufreq_device, cpu);
		else
			load = 0;

		total_load += load;
/*
		if (trace_thermal_power_cpu_limit_enabled() && load_cpu)
*/
		if (load_cpu)
			load_cpu[i] = load;

		i++;
	}

	cpufreq_device->last_load = total_load;

	dynamic_power = get_dynamic_power(cpufreq_device, freq);
	ret = get_static_power(cpufreq_device, tz, freq, &static_power);
	if (ret) {
		if (load_cpu)
			devm_kfree(&cdev->device, load_cpu);
		return ret;
	}

	if (load_cpu) {
		trace_thermal_power_cpu_get_power(
			&cpufreq_device->allowed_cpus,
			freq, load_cpu, i, dynamic_power, static_power);

        trace_IPA_actor_cpu_get_power(&cpufreq_device->allowed_cpus, freq,
				load_cpu, i, dynamic_power,
				static_power,(static_power + dynamic_power));

		devm_kfree(&cdev->device, load_cpu);
	}

	*power = static_power + dynamic_power;
	return 0;
}

/**
 * cpufreq_state2power() - convert a cpu cdev state to power consumed
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @state:	cooling device state to be converted
 * @power:	pointer in which to store the resulting power
 *
 * Convert cooling device state @state into power consumption in
 * milliwatts assuming 100% load.  Store the calculated power in
 * @power.
 *
 * Return: 0 on success, -EINVAL if the cooling device state could not
 * be converted into a frequency or other -E* if there was an error
 * when calculating the static power.
 */
static int cpufreq_state2power(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz,
			       unsigned long state, u32 *power)
{
	unsigned int freq, num_cpus;
	cpumask_t cpumask;
	u32 static_power, dynamic_power;
	int ret;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	cpumask_and(&cpumask, &cpufreq_device->allowed_cpus, cpu_online_mask);
	num_cpus = cpumask_weight(&cpumask);

	/* None of our cpus are online, so no power */
	if (num_cpus == 0) {
		*power = 0;
		return 0;
	}

	freq = get_cpu_frequency(cpumask_any(&cpumask), state);
	if (!freq)
		return -EINVAL;

	dynamic_power = cpu_freq_to_power(cpufreq_device, freq) * num_cpus;
	ret = get_static_power(cpufreq_device, tz, freq, &static_power);
	if (ret)
		return ret;

	*power = static_power + dynamic_power;
	return 0;
}

/**
 * cpufreq_power2state() - convert power to a cooling device state
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	power in milliwatts to be converted
 * @state:	pointer in which to store the resulting state
 *
 * Calculate a cooling device state for the cpus described by @cdev
 * that would allow them to consume at most @power mW and store it in
 * @state.  Note that this calculation depends on external factors
 * such as the cpu load or the current static power.  Calling this
 * function with the same power as input can yield different cooling
 * device states depending on those external factors.
 *
 * Return: 0 on success, -ENODEV if no cpus are online or -EINVAL if
 * the calculated frequency could not be converted to a valid state.
 * The latter should not happen unless the frequencies available to
 * cpufreq have changed since the initialization of the cpu cooling
 * device.
 */
static int cpufreq_power2state(struct thermal_cooling_device *cdev,
			       struct thermal_zone_device *tz, u32 power,
				   unsigned long *state)
{
	unsigned int cpu = 0;
	unsigned int cur_freq = 0;
	unsigned int target_freq = 0;
	int ret = 0;
	s32 dyn_power = 0;
	u32 last_load = 0;
	u32 normalised_power = 0;
	u32	static_power = 0;
	struct cpufreq_cooling_device *cpufreq_device = cdev->devdata;

	cpu = cpumask_any_and(&cpufreq_device->allowed_cpus, cpu_online_mask);

	/* None of our cpus are online */
	if (cpu >= nr_cpu_ids)
		return -ENODEV;

	cur_freq = cpufreq_quick_get(cpu);
	ret = get_static_power(cpufreq_device, tz, cur_freq, &static_power);
	if (ret)
		return ret;

	dyn_power = power - static_power;
	dyn_power = dyn_power > 0 ? dyn_power : 0;
	last_load = cpufreq_device->last_load ?: 1;
	normalised_power = (dyn_power * 100) / last_load;
	target_freq = cpu_power_to_freq(cpufreq_device, normalised_power);

	*state = cpufreq_cooling_get_level(cpu, target_freq);
	if (*state == THERMAL_CSTATE_INVALID) {
		dev_warn_ratelimited(&cdev->device,
				     "Failed to convert %dKHz for cpu %d into a cdev state\n",
				     target_freq, cpu);
		return -EINVAL;
	}

	trace_thermal_power_cpu_limit(&cpufreq_device->allowed_cpus,
				      target_freq, *state, power);
    trace_IPA_actor_cpu_limit(&cpufreq_device->allowed_cpus, target_freq,
					*state, power);
	return 0;
}

/* Bind cpufreq callbacks to thermal cooling device ops */
static struct thermal_cooling_device_ops cpufreq_cooling_ops = {
	.get_max_state = cpufreq_get_max_state,
	.get_cur_state = cpufreq_get_cur_state,
	.set_cur_state = cpufreq_set_cur_state,
};

/* Notifier for cpufreq policy change */
static struct notifier_block thermal_cpufreq_notifier_block = {
	.notifier_call = cpufreq_thermal_notifier,
};

/**
 * __cpufreq_cooling_register - helper function to create cpufreq cooling device
 * @np: a valid struct device_node to the cooling device device tree node
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 * @capacitance: dynamic power coefficient for these cpus
 * @plat_static_func: function to calculate the static power consumed by these
 *                    cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. It also gives the opportunity to link the cooling device
 * with a device tree node, in order to bind it via the thermal DT code.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
static struct thermal_cooling_device *
__cpufreq_cooling_register(struct device_node *np,
			const struct cpumask *clip_cpus, u32 capacitance,
			get_static_t plat_static_func)
{
	struct thermal_cooling_device *cool_dev;
	struct cpufreq_cooling_device *cpufreq_dev = NULL;
	unsigned int min = 0, max = 0;
	char dev_name[THERMAL_NAME_LENGTH];
	int ret = 0, i;
	unsigned int num_cpus;
	struct cpufreq_policy policy;

	/* Verify that all the clip cpus have same freq_min, freq_max limit */
	for_each_cpu(i, clip_cpus) {
		/* continue if cpufreq policy not found and not return error */
		if (!cpufreq_get_policy(&policy, i))
			continue;
		if (min == 0 && max == 0) {
			min = policy.cpuinfo.min_freq;
			max = policy.cpuinfo.max_freq;
		} else {
			if (min != policy.cpuinfo.min_freq ||
			    max != policy.cpuinfo.max_freq)
				return ERR_PTR(-EINVAL);
		}
	}
	cpufreq_dev = kzalloc(sizeof(struct cpufreq_cooling_device),
			      GFP_KERNEL);
	if (!cpufreq_dev)
		return ERR_PTR(-ENOMEM);

	num_cpus = cpumask_weight(clip_cpus);
	cpufreq_dev->time_in_idle = kcalloc(num_cpus,
					    sizeof(*cpufreq_dev->time_in_idle),
					    GFP_KERNEL);
	if (!cpufreq_dev->time_in_idle) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto free_cdev;
	}

	cpufreq_dev->time_in_idle_timestamp =
		kcalloc(num_cpus, sizeof(*cpufreq_dev->time_in_idle_timestamp),
			GFP_KERNEL);
	if (!cpufreq_dev->time_in_idle_timestamp) {
		cool_dev = ERR_PTR(-ENOMEM);
		goto free_time_in_idle;
	}

	cpumask_copy(&cpufreq_dev->allowed_cpus, clip_cpus);

	if (capacitance) {
		cpufreq_cooling_ops.get_requested_power =
			cpufreq_get_requested_power;
		cpufreq_cooling_ops.state2power = cpufreq_state2power;
		cpufreq_cooling_ops.power2state = cpufreq_power2state;
		cpufreq_dev->plat_get_static_power = plat_static_func;

		ret = build_dyn_power_table(cpufreq_dev, capacitance);
		if (ret) {
			cool_dev = ERR_PTR(ret);
			goto free_time_in_idle_timestamp;
		}
	}

	ret = get_idr(&cpufreq_idr, &cpufreq_dev->id);
	if (ret) {
		cool_dev = ERR_PTR(-EINVAL);
		goto free_time_in_idle_timestamp;
	}

	snprintf(dev_name, sizeof(dev_name), "thermal-cpufreq-%d",
		 cpufreq_dev->id);

	cool_dev = thermal_of_cooling_device_register(np, dev_name, cpufreq_dev,
						      &cpufreq_cooling_ops);
	if (IS_ERR(cool_dev))
		goto remove_idr;
	cpufreq_dev->cool_dev = cool_dev;
	cpufreq_dev->cpufreq_state = 0;
	mutex_lock(&cooling_cpufreq_lock);

	/* Register the notifier for first cpufreq cooling device */
	if (cpufreq_dev_count == 0)
		cpufreq_register_notifier(&thermal_cpufreq_notifier_block,
					  CPUFREQ_POLICY_NOTIFIER);
	cpufreq_dev_count++;

	mutex_unlock(&cooling_cpufreq_lock);

	return cool_dev;

remove_idr:
	release_idr(&cpufreq_idr, cpufreq_dev->id);
free_time_in_idle_timestamp:
	kfree(cpufreq_dev->time_in_idle_timestamp);
free_time_in_idle:
	kfree(cpufreq_dev->time_in_idle);
free_cdev:
	kfree(cpufreq_dev);

	return cool_dev;
}

/**
 * cpufreq_cooling_register - function to create cpufreq cooling device.
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus)
{
	return __cpufreq_cooling_register(NULL, clip_cpus, 0, NULL);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_register);

/**
 * of_cpufreq_cooling_register - function to create cpufreq cooling device.
 * @np: a valid struct device_node to the cooling device device tree node
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen.
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. Using this API, the cpufreq cooling device will be
 * linked to the device tree node provided.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
of_cpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_cpus)
{
	if (!np)
		return ERR_PTR(-EINVAL);

	return __cpufreq_cooling_register(np, clip_cpus, 0, NULL);
}
EXPORT_SYMBOL_GPL(of_cpufreq_cooling_register);

/**
 * cpufreq_power_cooling_register() - create cpufreq cooling device with power extensions
 * @clip_cpus:	cpumask of cpus where the frequency constraints will happen
 * @capacitance:	dynamic power coefficient for these cpus
 * @plat_static_func:	function to calculate the static power consumed by these
 *			cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with
 * the name "thermal-cpufreq-%x".  This api can support multiple
 * instances of cpufreq cooling devices.  Using this function, the
 * cooling device will implement the power extensions by using a
 * simple cpu power model.  The cpus must have registered their OPPs
 * using the OPP library.
 *
 * An optional @plat_static_func may be provided to calculate the
 * static power consumed by these cpus.  If the platform's static
 * power consumption is unknown or negligible, make it NULL.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpufreq_power_cooling_register(const struct cpumask *clip_cpus, u32 capacitance,
			       get_static_t plat_static_func)
{
	return __cpufreq_cooling_register(NULL, clip_cpus, capacitance,
				plat_static_func);
}
EXPORT_SYMBOL(cpufreq_power_cooling_register);

/**
 * of_cpufreq_power_cooling_register() - create cpufreq cooling device with power extensions
 * @np:	a valid struct device_node to the cooling device device tree node
 * @clip_cpus:	cpumask of cpus where the frequency constraints will happen
 * @capacitance:	dynamic power coefficient for these cpus
 * @plat_static_func:	function to calculate the static power consumed by these
 *			cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with
 * the name "thermal-cpufreq-%x".  This api can support multiple
 * instances of cpufreq cooling devices.  Using this API, the cpufreq
 * cooling device will be linked to the device tree node provided.
 * Using this function, the cooling device will implement the power
 * extensions by using a simple cpu power model.  The cpus must have
 * registered their OPPs using the OPP library.
 *
 * An optional @plat_static_func may be provided to calculate the
 * static power consumed by these cpus.  If the platform's static
 * power consumption is unknown or negligible, make it NULL.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
of_cpufreq_power_cooling_register(struct device_node *np,
				  const struct cpumask *clip_cpus,
				  u32 capacitance,
				  get_static_t plat_static_func)
{
	if (!np)
		return ERR_PTR(-EINVAL);

	return __cpufreq_cooling_register(np, clip_cpus, capacitance,
				plat_static_func);
}
EXPORT_SYMBOL(of_cpufreq_power_cooling_register);

/**
 * cpufreq_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 *
 * This interface function unregisters the "thermal-cpufreq-%x" cooling device.
 */
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct cpufreq_cooling_device *cpufreq_dev;

	if (!cdev)
		return;

	cpufreq_dev = cdev->devdata;
	mutex_lock(&cooling_cpufreq_lock);
	cpufreq_dev_count--;

	/* Unregister the notifier for the last cpufreq cooling device */
	if (cpufreq_dev_count == 0)
		cpufreq_unregister_notifier(&thermal_cpufreq_notifier_block,
					    CPUFREQ_POLICY_NOTIFIER);
	mutex_unlock(&cooling_cpufreq_lock);

	thermal_cooling_device_unregister(cpufreq_dev->cool_dev);
	release_idr(&cpufreq_idr, cpufreq_dev->id);
	kfree(cpufreq_dev->time_in_idle_timestamp);
	kfree(cpufreq_dev->time_in_idle);
	kfree(cpufreq_dev);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_unregister);
