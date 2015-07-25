clc
clear

Y = wavread('miracle.wav',[1 2048]);
Y2 = int64(32767 * Y);

acf = transpose(autocorl(Y,20));
ref = schurrc(acf);
qtz_ref = qtz_par(ref);%Transmit quantized reflection coeffs

lpc_coeffs = levinson(acf,13);
coeffs = int64(2^20 * lpc_coeffs);
res = calc_residue(Y2,[0 -coeffs(2:end)],20);
subplot(2,1,1);
plot(Y2(1:200));
hold on;
plot(res(1:200),'green');

y = calc_original(res,[0 -coeffs(2:end)],20);
y = transpose(y);
subplot(2,1,2)
plot(y(1:200));
diff = Y2 - int64(y);
