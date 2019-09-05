
%% INIT parameters
clc; clear all;

[mem_size, mem_exec_times, shm_exec_times] = import_data();

min_mem_exec_time = min(mem_exec_times');
avg_mem_exec_time = mean(mem_exec_times');
max_mem_exec_time = max(mem_exec_times');

min_shm_exec_time = min(shm_exec_times');
avg_shm_exec_time = mean(shm_exec_times');
max_shm_exec_time = max(shm_exec_times');

size_labels={};
for i = 1:31
	if(i<11)
		size_labels{i} = string(2^(i-1))+"B";		
	elseif(i<21)
		size_labels{i} = string(2^(i-1-10))+"KB";
	elseif(i<31)
		size_labels{i} = string(2^(i-1-20))+"MB";
	else
		size_labels{i} = string(2^(i-1-30))+"GB";
	end
end
clear i;

%% MEM Full Min, Avg, Max
figure;
hold on;
grid on;
title("Execution time using buffers");
xlabel("size");
ylabel("Time in us");

range = 1:size(min_mem_exec_time, 2);

y_min = min(min_mem_exec_time);
y_max = max(max_mem_exec_time);
y_step=50;

xticks(range);
xticklabels(size_labels(range))
yticks(floor(y_min):y_step:ceil(y_max));


plot(min_mem_exec_time);
plot(avg_mem_exec_time);
plot(max_mem_exec_time);

legend({'min','average','max'});


%% MEM 1 to 16KB Min, Avg
figure;
hold on;
grid on;
title("Execution time using buffers");
xlabel("size");
ylabel("Time in us");

range = 1:16;
y_step = 0.5;

y_min = floor(min(min_mem_exec_time(range)));
y_max = ceil(max(max_mem_exec_time(range)));

xticks(range);
xticklabels(size_labels(range))
yticks(y_min:y_step:y_max);

plot(min_mem_exec_time(range));
plot(avg_mem_exec_time(range));

legend({'min','average'});

%% MEM all case all iteration
figure;
hold on;
grid on;

title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Time in us (log scale)");

range = 1:size(mem_exec_times, 1);

for i = range
	plot(mem_exec_times(i, :));
end

set(gca, 'YScale', 'log');
legend(size_labels(range));


%% MEM 32KB strange case
figure;
hold on;
grid on;

title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Time in us (log scale)");

i = 16;
range = i;

plot(mem_exec_times(i, :));

set(gca, 'YScale', 'log');
legend(size_labels(range));


%% MEM Histograms
figure;
hold on;
title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Distribution");

x_max = 0;
x_min = 10000;
x_step = 1;

range = 1:1:8;

for i=range
	histogram(mem_exec_times(i,:), 100,'Normalization','probability');
	x_max = max(x_max, max_mem_exec_time(i));
	x_min = min(x_min, min_mem_exec_time(i));
end

legend(size_labels(range));
x_max = 30;
xlim([x_min x_max]);
xticks(floor(x_min):x_step:ceil(x_max));


%% MEM Histograms
figure;
hold on;
title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Distribution");

x_max = 0;
x_min = 10000;
x_step = 1;

range = 8:1:14;

for i=range
	histogram(mem_exec_times(i,:), 100,'Normalization','probability');
	x_max = max(x_max, max_mem_exec_time(i));
	x_min = min(x_min, min_mem_exec_time(i));
end

legend(size_labels(range));
xlim([x_min x_max]);
xticks(floor(x_min):x_step:ceil(x_max));

%% MEM Histograms
figure;
hold on;
title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Distribution");

x_max = 0;
x_min = 10000;
x_step = 50;

range = 15:1:21;

for i=range
	histogram(mem_exec_times(i,:), 100,'Normalization','probability');
	x_max = max(x_max, max_mem_exec_time(i));
	x_min = min(x_min, min_mem_exec_time(i));
end

legend(size_labels(range));
xlim([x_min x_max]);
xticks(floor(x_min):x_step:ceil(x_max));

%% MEM Histograms full
figure;
hold on;
title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Distribution");

x_max = 0;
x_min = 10000;
x_step = 50;

range = 1:1:21;

for i=range
	histogram(mem_exec_times(i,:), 100,'Normalization','probability');
	x_max = max(x_max, max_mem_exec_time(i));
	x_min = min(x_min, min_mem_exec_time(i));
end

legend(size_labels(range));
xlim([x_min x_max]);
xticks(floor(x_min):x_step:ceil(x_max));
set(gca, 'XScale', 'log');

%% plot shared mem min, avg, max
figure;
hold on;
grid on;
title("Execution time using shared memory");
xlabel("size");
ylabel("Time in us");

range = 1:size(min_shm_exec_time, 2);

y_min = min(min_shm_exec_time);
y_max = max(max_shm_exec_time);
y_step=50;

xticks(range);
xticklabels(size_labels(range))
yticks(floor(y_min):y_step:ceil(y_max));


plot(min_shm_exec_time);
plot(avg_shm_exec_time);
plot(max_shm_exec_time);

legend({'min','average','max'});

%% plot shared mem min, avg
figure;
hold on;
grid on;
title("Execution time using shared memory");
xlabel("size");
ylabel("Time in us");

range = 1:size(min_shm_exec_time, 2);

y_min = min(min_shm_exec_time);
y_max = max(max_shm_exec_time);
y_step=0.5;

xticks(range);
xticklabels(size_labels(range))
yticks(floor(y_min):y_step:ceil(y_max));


plot(min_shm_exec_time);
plot(avg_shm_exec_time);

legend({'min','average'});


%% SHM all case all iteration
figure;
hold on;
grid on;

title("Execution time using shared memory");
xlabel("Iteration #");
ylabel("Time in us (log scale)");

range = 1:size(shm_exec_times, 1);

for i = range
	plot(shm_exec_times(i, :));
end

legend(size_labels(range));




%% SHM Histogram
figure;
hold on;
grid on;
title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Distribution");

x_max = 0;
x_min = 10000;
x_step = 2;

range = 1:1:31;

for i=range
	histogram(shm_exec_times(i,:), 100,'Normalization','probability');
	x_max = max(x_max, max_shm_exec_time(i));
	x_min = min(x_min, min_shm_exec_time(i));
end

legend(size_labels(range));
xlim([x_min 100]);
xticks(floor(x_min):x_step:ceil(x_max));


