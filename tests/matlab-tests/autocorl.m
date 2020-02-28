function acf = autocorl(x,n)
    acf = zeros(1,n+1);
    sum = 0;
    for i=1:length(x);
        sum = sum + x(i);
    end
    mean = sum / length(x);
    
    for i=1:n+1
        for j = i:length(x)
            acf(i) = acf(i) + (x(j) - mean) * (x(j-i+1) - mean);
        end
    end
    
    for i=2:n+1
        acf(i) = acf(i)/acf(1);
    end
    acf(1) = 1;
end
