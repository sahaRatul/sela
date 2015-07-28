clc
clear

%Change the wav file name to suit your purpose
Y = wavread('miracle.wav',[1 2048]);
Y2 = int64(32767 * Y);

acf = transpose(autocorl(Y,60));
ref2 = schurrc(acf);%Calculate reflection coefficients

%Calculate optimal order
opt_order = 1;
for i = length(ref2):-1:1
    if(abs(ref2(i)) > 0.1)
        opt_order = i + 1;
        break;
    end
end

%Quantize coefficients
qtz_ref = qtz_par(ref2);

%Dequantize
ref = dqtz_par(qtz_ref,opt_order);

%Reflection to lpc
lpc_coeffs = rc2poly(ref);

%Scale coefficients
coeffs = int64(2^25 * lpc_coeffs);

%Calculate residue
res = calc_residue(Y2,[0 -coeffs(2:end)],25);
subplot(2,1,1);
plot(Y2(1:200));
hold on;
plot(res(1:200),'green');

%Calculate original
y = calc_original(res,[0 -coeffs(2:end)],25);
y = transpose(y);
subplot(2,1,2)
plot(y(1:200));
diff = Y2 - int64(y);

avg = 0;
for i=1:length(res)
    avg = avg + res(i);
end

avg = avg/length(res);
ref = transpose(ref);
lpc_coeffs = transpose(lpc_coeffs);
coeffs = transpose(coeffs);
res = transpose(res);
