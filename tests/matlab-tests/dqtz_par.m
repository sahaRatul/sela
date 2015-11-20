function ref = dqtz_par(coff,opt_order)
    ref = zeros(1,opt_order);
    
    if (opt_order <= 1)
      ref(1) = 0;
    else
      ref(1) = (((coff(1)/64+1)/sqrt(2))^2)-1;
      ref(2) = -((((coff(2)/64+1)/sqrt(2))^2)-1);
      for i=3:opt_order
        ref(i) = coff(i)/64;
      end
    end
end
