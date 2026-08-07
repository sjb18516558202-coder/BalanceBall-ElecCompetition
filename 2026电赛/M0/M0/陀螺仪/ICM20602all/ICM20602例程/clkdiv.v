module clkdiv
(
	input clk,
	input [11:0] div,
	output wire clkdiv
);

	reg [25:0]Count;
	reg rclkdiv;
	
always @ (posedge clk)
	begin  
		if(div[0]==1'b0) // 如果N为偶数，比N%2==0这种判断方式更节省资源
			begin
				if(div==2) // 如果N为2
					rclkdiv<= ~rclkdiv;
				else
					if(Count==((div-2)>>1'b1))   //比cnt1==(N-2)/2这种判断方式更节省资源
						begin
							Count<= 1'b0;
							rclkdiv<= ~rclkdiv;
						end
					else
						Count<=Count+1'b1;
			end
		else // 如果N为奇数
			if(Count==(div-1'b1)>>2)
				begin
					rclkdiv<= ~rclkdiv;
					Count<=Count+1'b1;
				end
			else if(Count==div-1'b1) 
				begin 
					Count<= 1'b0;
					rclkdiv<= ~rclkdiv;
				end
			else
				Count<=Count+1'b1;
	end
		
assign  clkdiv=rclkdiv;

endmodule 		