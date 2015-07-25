function residues = calc_residue(samples,lpc_coeffs,Q)
    N = length(samples);
    ord = length(lpc_coeffs);
    disp(ord);
    residues = zeros(1,N);
    
    residues(1) = samples(1);
    
    for k = 2:ord
        y = 2^(Q-1);
        for i = 2:k+1
            y = y + (lpc_coeffs(i) * samples(k-i+1));
            if((k - i)==0)
                break;
            end
        end
        residues(k) = samples(k) - y/2^Q;
    end
    
    
    for k = ord+1:N
        y = 2^(Q-1);
        for i = 1:ord
            y = y + (lpc_coeffs(i) * samples(k-i+1));
        end
        residues(k) = y/2^Q;
    end
end
