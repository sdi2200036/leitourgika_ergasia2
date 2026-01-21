#!/bin/sh

DAEMONS=$((`nproc` - 2))
HEAVY=$((`nproc` / 2))

PIDS=""
i=1
while [ "$i" -le "$DAEMONS" ]; do
	./daemon &
	PIDS=`echo -n "$PIDS $!"`
	i=$((i+1))
done;

i=2
max=$((`nproc` - 1))

while [ "$i" -le "$max" ]; do
	sudo ./assign_ncores_to_group $i D
	echo "========== GRR DEFAULT GROUP CPUS: $i | GRR PERFORMANCE GROUP CPUS: $(( max + 1 - i )) =========="

	echo -n "Daemons are running on cpus: "
	sep=""
	for pid in $PIDS; do
		cpu=$(awk '{print $39}' /proc/$pid/stat)
		printf "%s%s" "$sep" "$cpu"
		sep=", "
	done
	echo ""

	echo "========== DEFAULT =========="
	sudo ./heavy D $HEAVY

	echo "========== PERFORMANCE =========="
	sudo ./heavy P $HEAVY
	i=$((i+2))

	echo ""
done

for pid in $PIDS; do
	kill -9 $pid
done
