% This program is an implementation of IIR-Filter

X = 1; % Input-Signal
Y = []; % Vector of Output-Signal
Y_Temp = 0; % Current Output-Signal

% Konstante
Alpha_2 = 0;
Alpha_1 = 1; 
Alpha_0 = 1;
Beta_2 = 1;
Beta_1 = -1.4142;

% Calculation of Output-Signal

for i = 0:1:100
    if i == 0
        Y_Temp = Alpha_0*X;
    end
    
    if i == 1
        Y_Temp = Alpha_1*X - Beta_1*Y(1,i); % i = n-1
    end
    if i > 1
        Y_Temp = -Beta_1*Y(1,i) - Beta_2*Y(1,i-1); % i = n-1
    end
    Y = [Y Y_Temp];
end

i = 0:1:100;
stem(i,Y);
grid on;
