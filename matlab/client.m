function client(port)
%   provides a menu for accessing PIC32 motor control functions
%
%   client(port)
%
%   Input Arguments:
%       port - the name of the com port.  This should be the same as what
%               you use in screen or putty in quotes ' '
%
%   Example:
%       client('/dev/ttyUSB0') (Linux/Mac)
%       client('COM3') (PC)
%
%   For convenience, you may want to change this so that the port is hardcoded.
   
% Opening COM connection
if ~isempty(instrfind)
    fclose(instrfind);
    delete(instrfind);
end

fprintf('Opening port %s....\n',port);

% settings for opening the serial port. baud rate 230400, hardware flow control
% wait up to 120 seconds for data before timing out
mySerial = serial(port, 'BaudRate', 230400, 'FlowControl', 'hardware','Timeout',120); 
% opens serial connection
fopen(mySerial);
% closes serial port when function exits
clean = onCleanup(@()fclose(mySerial));                                 

has_quit = false;
% menu loop
while ~has_quit
    fprintf('PIC32 MOTOR DRIVER INTERFACE\n\n');
    % display the menu options; this list will grow
    fprintf('\ta: Read current sensor (ADC counts)\tb: Readcurrent sensor (mA)\n');
    fprintf('\tc: Read encoder (counts)\t\td: Read encoder (deg)\n');
    fprintf('\te: Reset encoder\t\t\tf: Set PWM (-100 to 100)\n');
    fprintf('\tg: Set current gains\t\t\th: Get current gains\n');
    fprintf('\ti: Set position gains\t\t\tj: Get position gains\n');
    fprintf('\tk: Test current control\t\t\tl: Go to angle (deg)\n');
    fprintf('\tm: Load step trajectory\t\t\tn: Load cubic trajectory\n');
    fprintf('\to: Execute trajectory\t\t\tp: Unpower the motor\n');
    fprintf('\tq: Quit client\t\t\t\tr: Get mode\n');
    
    % read the user's choice
    selection = input('\nENTER COMMAND: ', 's');
    clc; %clear the screen for next command
    fprintf(mySerial,'%c\n',selection);
    
    % take the appropriate action
    switch selection
        case 'a'  % Motor current ADC count
            counts = fscanf(mySerial, '%d');
            fprintf('The motor current is %d ADC counts.\n\n', counts);
        
        case 'b'  % Motor current sensor mA
            counts = fscanf(mySerial, '%d');
            fprintf('The motor current is %d mA.\n\n', counts);
        
        case 'c' % Motor angle in counts
            counts = fscanf(mySerial, '%d');
            fprintf('The motor angle is %d counts.\n\n', counts);
        
        case 'd'  % Motor angle in degrees
            degrees = fscanf(mySerial, '%f');
            fprintf('The motor angle is %f degrees.\n\n', degrees);
        
        case 'e'    %Reset encoder counts  
            fprintf('The motor angle is 0 degrees.\n\n');
        
        case 'f'  %Set PWM (-100 to 100)
            n = input('What PWM value would you like [-100 to 100] : '); % get the PWM to send
            while n>100 || n <-100
                n = input('Thats not in the possible range, please enter a PWM [-100 to 100] : ');
            end
            fprintf(mySerial, '%d\n',n); % send the number
            if n>0
                fprintf('PWM has been set to %d%% in the counterclockwise direction\n\n',n); % print to the screen
            end
            if n<0
                fprintf('PWM has been set to %d%% in the clockwise direction\n\n',abs(n)); % print to the screen
            end
            
        case 'g' % Set current gains
            Kp = input('Enter your desired Kp current gain [recommended: 0.3]: ');
            Ki = input('Enter your desired Ki current gain [recommended: 0.005]: ');
            fprintf(mySerial, '%f %f\r\n',[Kp,Ki]); % send the number            
            %fprintf('Sending Kp = %f and Ki = %f to the current controller.\n\n',Kp, Ki); % print to the screen
        
        case 'h' % Get current gains
            data = fscanf(mySerial, '%f %f');
            fprintf('The current controller is using Kp = %3.3f and Ki = %3.3f\n\n',[data(1), data(2)]); % print to the screen    
        
        case 'i' % Set position gains
            PKp = input('Enter your desired Kp position gain [recommended: 75]: ');
            PKi = input('Enter your desired Ki position gain [recommended: 0]: ');
            PKd = input('Enter your desired Kd position gain [recommended: 5000]: ');
            fprintf(mySerial, '%f %f %f\r\n',[PKp,PKi,PKd]); % send the number            
            %fprintf('Sending Kp = %f and Ki = %f to the current controller.\n\n',Kp, Ki); % print to the screen
        
        case 'j' % Get position gains
            data2 = fscanf(mySerial, '%f %f %f');
            fprintf('The position controller is using Kp = %3.3f, Ki = %3.3f, and Kd = %3.3f\n\n',[data2(1), data2(2), data2(3)]); % print to the screen    
        
        case 'k' % Test Current control
            fprintf('Testing current control.\nPlease hold...\n');
            read_plot_matrix(mySerial);
        
        case 'l' % Go to angle (deg)
            n = input('Enter the desired motor angle in degrees: '); % get the desired angle
            fprintf(mySerial, '%d\n',n); % send the angle
            fprintf('Motor moving to %d degrees.\n\n', n);
        
        case 'm' % Load step Trajectory
            A = input('Enter step trajectory, in sec and degrees [time1, ang1; time2, ang2; ...]: ');
            while A(end, 1)>10
                fprintf('Error: Maximum trajectory time is 10 seconds');
                A = input('Enter step trajectory, in sec and degrees [time1, ang1; time2, ang2; ...]: ');
            end
            ref = genRef(A, 'step');
           
            fprintf(mySerial, '%d\n',size(ref,2)); % send the number of samples
            for i = 1:size(ref,2)
                fprintf(mySerial, '%d\n',ref(i)); % send the reference array
            end
            
        case 'n' % Load Cubic Trajectory
            
            A = input('Enter cubic trajectory, in sec and degrees [time1, ang1; time2, ang2; ...]: ');
            while A(end, 1)>10
               fprintf('Error: Maximum trajectory time is 10 seconds');
               A = input('Enter cubic trajectory, in sec and degrees [time1, ang1; time2, ang2; ...]: ');
            end
            ref = genRef(A, 'cubic');
           
            fprintf(mySerial, '%d\n',size(ref,2)); % send the number of samples
            for i = 1:size(ref,2)
                fprintf(mySerial, '%d\n',ref(i)); % send the reference array
            end
            
        case 'o' % Execute Trajectory
            fprintf('Following set trajectory.\nPlease hold...\n');
            plot_trajectory(mySerial);
        
        case 'p' %unpower the moter
            fprintf('Motor unpowered, set to IDLE mode\n\n');
        
        case 'q'
            clc;
            fprintf('Thank you for playing!\n\n');
            has_quit = true;             % exit client
        
        case 'r' %get mode
            mode = fscanf(mySerial, '%d');
            if mode == 1
                fprintf('The PIC32 controller mode is currently IDLE\n\n');
            end
            
            if mode == 2
                fprintf('The PIC32 controller mode is currently PWM\n\n');
            end
            
            if mode == 3
                fprintf('The PIC32 controller mode is currently ITEST\n\n');
            end
            
            if mode == 4
                fprintf('The PIC32 controller mode is currently HOLD\n\n');
            end
            
            if mode == 5
                fprintf('The PIC32 controller mode is currently TRACK\n\n');
            end
        otherwise
            fprintf('Invalid Selection %c\n', selection);
    end
    
end

end

