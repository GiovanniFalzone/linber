%% INIT parameters
clc;
clear all;

[mem_size, mem_exec_times, shm_exec_times] = import_data();
mem_exec_times = mem_exec_times / 1000;
shm_exec_times = shm_exec_times / 1000;

figure;
subplot(2, 1, 1);
surf(mem_exec_times);
subplot(2, 1, 2);
surf(shm_exec_times);

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

%% Mem vs Shm
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Average execution time");
xlabel("size");
ylabel("Time in us");

range = 1:17;
y_step = 1;

y_min = floor(min(min_mem_exec_time(range)));
y_max = ceil(max(max_mem_exec_time(range)));

xticks(range);
xticklabels(size_labels(range))
yticks(y_min:y_step:y_max);

plot(avg_mem_exec_time(range));
plot(avg_shm_exec_time(range));

legend({'mem','shm'});

filename = 'mem_vs_shm_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, '../plots/'+filename+'.png');


%% Memory buffer
dir_path = '../plots/mem/';

%% MEM Full Min, Avg, Max
figure('units','normalized','outerposition',[0 0 1 1]);
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


filename = 'min_avg_max_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% MEM 1 to 16KB Min, Avg
figure('units','normalized','outerposition',[0 0 1 1]);
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

filename = 'min_avg_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');

%% MEM all case all iteration
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;

title("Execution time using buffers");
xlabel("Iteration #");
ylabel("Time in us (log scale)");
set(gca, 'YScale', 'log');
range = 1:size(mem_exec_times, 1);

for i = range
	plot(mem_exec_times(i, :));
end
legend(size_labels(range));

filename = 'exec_time_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');

%% MEM single Histograms
my_dir_path = dir_path + "hist/";

range = 1:size(mem_exec_times, 1);

for i=range
	figure('units','normalized','outerposition',[0 0 1 1]);
	grid on;

	histogram(mem_exec_times(i,:), 100,'Normalization','probability');

	title("Execution time using buffers");
	xlabel("Time in us");
	ylabel("Probability");
	
	legend(size_labels(i));
	filename = 'hist_' + string(size_labels(range(i)));
	saveas(gcf, my_dir_path + filename+'.png');
end


%% MEM Histograms
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Execution time using buffers");
xlabel("Time in us");
ylabel("Probability");

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

filename = 'hist_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% MEM Histograms
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Execution time using buffers");
xlabel("Time in us");
ylabel("Probability");

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

filename = 'hist_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');

%% MEM Histograms
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Execution time using buffers");
xlabel("Time in us");
ylabel("Probability");

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

filename = 'hist_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% MEM Histograms full
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Execution time using buffers");
xlabel("Time in us (log scale)");
xtickangle(90);
set(gca, 'XScale', 'log');
ylabel("Probability");

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

filename = 'hist_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% Shared memory
dir_path = '../plots/shm/';

%% plot shared mem min, avg, max
figure('units','normalized','outerposition',[0 0 1 1]);
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

filename = 'min_avg_max_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% plot shared mem min, avg
figure('units','normalized','outerposition',[0 0 1 1]);
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

filename = 'min_avg_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');


%% SHM all case all iteration
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;

title("Execution time using shared memory");
xlabel("Iteration #");
ylabel("Time in us");

y_max = 0;
y_min = 10000;
y_step = 20;
range = 1:size(shm_exec_times, 1);

for i = range
	y_max = max_shm_exec_time(i);
	y_min = min_shm_exec_time(i);
	plot(shm_exec_times(i, :));
end

legend(size_labels(range));

ylim([y_min y_max]);
yticks(floor(y_min):y_step:ceil(y_max));


filename = 'exec_time_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');

%% SHM single Histograms
my_dir_path = dir_path + "hist/";

range = 1:size(shm_exec_times, 1);

for i=range
	figure('units','normalized','outerposition',[0 0 1 1]);
	grid on;

	histogram(shm_exec_times(i,:), 100,'Normalization','probability');

	title("Execution time using buffers");
	xlabel("Time in us");
	ylabel("Probability");
	
	legend(size_labels(i));
	filename = 'hist_' + string(size_labels(range(i)));
	saveas(gcf, my_dir_path + filename+'.png');
end


%% SHM Histogram
figure('units','normalized','outerposition',[0 0 1 1]);
hold on;
grid on;
title("Execution time using buffers");
xlabel("Time in us");
ylabel("Probability");

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

filename = 'hist_' + string(size_labels(range(1))) + '_to_' + string(size_labels(range(size(range, 2))));
saveas(gcf, dir_path + filename+'.png');

%% end
close all;
