% This program is an implementation of IIR-Filter
f = 100; % Frequency in Hz
fa = 44000;
ohm = 2*pi*f/fa;

X = 1; % Input-Signal
Y = []; % Vector of Output-Signal
Y_Temp = 0; % Current Output-Signal

% Konstante
Alpha_2 = 0;
Alpha_1 = 1; 
Alpha_0 = 1;
Beta_2 = 1;
Beta_1 = cos(ohm)*2;

% Calculation of Output-Signal

for i = 0:1:500
    if i == 0
        Y_Temp = 0;
    end
    
    if i == 1
        Y_Temp = 0; % i = n-1
    end
    if i == 2
        Y_Temp = +Beta_1*Y(1,i) - Beta_2*Y(1,i-1) + X; % i = n-1
    end
    
    if i > 2
        Y_Temp = +Beta_1*Y(1,i) - Beta_2*Y(1,i-1);
    end
    Y = [Y Y_Temp];
end

i = 0:1:500;
i = i./fa;
stem(i,Y);
xlabel('time in s');
grid on;
