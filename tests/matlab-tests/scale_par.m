function scaled = scale_par(Q,qtz)
    n = length(qtz);
    scaled = zeros(n,1);
    for i = 1:n
        scaled(i) = qtz(i)*2^(Q-6) + 2^(Q-7);
    end
end
