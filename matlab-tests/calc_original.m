function original = calc_original(residues,lpc_coeffs,Q)
    N = length(residues);
    ord = length(lpc_coeffs);
    disp(ord);
    original = zeros(1,N);
    
    original(1) = residues(1);
    for k = 1:N
        y = 2^(Q-1);
        for i = 1:ord
            y = y - (lpc_coeffs(i) * original(k-i+1));
            if(k-i) == 0
                break;
            end
        end
        original(k) = residues(k) - y/2^Q;
    end
end
