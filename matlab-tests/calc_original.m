function original = calc_original(residues,lpc_coeffs,Q)
    N = length(residues);
    ord = length(lpc_coeffs);
    disp(ord);
    original = zeros(1,N);
    
    original(1) = residues(1);
    
    for k = 2:ord
        y = 2^(Q-1);
        for i = 2:k+1
            y = y - (lpc_coeffs(i) * original(k-i));
            if((k - i)==1)
                break;
            end
        end
        original(k) = residues(k) - y/2^Q;
    end
    
    
    for k = ord+1:N
        y = 2^(Q-1);
        for i = 1:ord
            y = y - (lpc_coeffs(i) * original(k-i));
        end
        original(k) = residues(k) - y/2^Q;
    end
end
