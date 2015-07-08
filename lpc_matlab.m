clc
clear

%%	Get original signal
[Y,FS,Nbits] = wavread('3steps.wav',[1 200]);
subplot(3,1,1)
plot(Y);

%Get autocorrelation data
acf = autocorr(Y,20);

%Get linear prediction coefficients
coff=levinson(acf,20);

%%Determine estimate of original signal Y
est_Y=filter([0 -coff(2:end)],1,Y);
subplot(3,1,2)
plot(est_Y);

%Get residues
residue = Y-est_Y;

%Get back original signal from residues
rcv = filter(1,coff,residue);
subplot(3,1,3);
plot(rcv);
