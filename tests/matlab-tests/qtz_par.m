function qtz = qtz_par(coff)
    n = length(coff);
    qtz = zeros(n,1);
    for i=1:n
        if i==1
            qtz(i) = floor(64*(-1 + sqrt(2)*sqrt(coff(i)+1)));
        elseif i==2
            qtz(i) = floor(64*(-1 + sqrt(2)*sqrt(-coff(i)+1)));
        else
            qtz(i) = floor(64*coff(i));
        end
    end
end
