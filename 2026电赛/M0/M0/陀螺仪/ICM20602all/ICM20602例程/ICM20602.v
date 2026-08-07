/*
 *  ICM20602.v
 *
 *  Created on: 2019年10月19日
 *  Author: 63
 */
 
module ICM20602(
input  CLK_50M,
input  wire rst_n,
output wire SCLK,
output MOSI,	
input  MISO,
output reg CS,
output reg [15:0] outrddat,
input datacs,
input RDdata,
input [11:0] dataAddr
);

wire CLK_10M;
////////////////msp430 read data//////////////////
reg [15:0]cnt=16'd0;
reg start=0;
wire done;

reg [15:0] rddat0,rddat1,rddat2,rddat3,rddat4,rddat5;
reg [15:0] rddat6,rddat7,rddat8,rddat9,rddat10,rddat11,rddat12;

reg [15:0] wrdat_16;
wire [15:0] rddat_16;
wire SCLK_16;
wire MOSI_16;
wire done_16;
reg start_16=1'd0;
assign MOSI=MOSI_16;
assign SCLK=SCLK_16;


clkdiv clkdiv_inst(
.clk(CLK_50M),
.div(16'd10),
.clkdiv(CLK_10M)
);

wire clk4test;
clkdiv clkdiv_inst1(
.clk(CLK_50M),
.div(16'd1000),
.clkdiv(clk4test)
);

wire clk4test2;
clkdiv clkdiv_inst2(
.clk(clk4test),
.div(16'd50),
.clkdiv(clk4test2)
);

wire clk4test3;
clkdiv clkdiv_inst3(
.clk(CLK_50M),
.div(16'd50),
.clkdiv(clk4test3)
);

////////////////read data//////////////////
always@(posedge CLK_50M or negedge rst_n)
begin
	if(!rst_n)
		outrddat<=16'd0;
	else if(datacs&RDdata)
	begin
		case(dataAddr[7:0])
		8'd0:outrddat<=result0;
		8'd1:outrddat<=result1;
		8'd2:outrddat<=result2;
		8'd3:outrddat<=result3;
		8'd4:outrddat<=result4;
		8'd5:outrddat<=result5;
		default: outrddat<=16'd0;
		endcase
	end
end

ICM_16SPI ICM_16SPI_inst
(
	.clk(CLK_10M) ,	// input  clk_sig
	.rst_n(rst_n) ,	// input  rst_n_sig
	.go(start_16) ,	// input  start_sig
	.wrdat(wrdat_16) ,	// input [7:0] wrdat_sig
	.rddat(rddat_16) ,	// output [7:0] rddat_sig
	.ok(done_16),
	.mosi(MOSI_16) ,	// input  MISO_sig
	.sclk(SCLK_16) ,	// output  MOSI_sig
	.miso(MISO)
);

reg state0=1'd0,state1=1'd0;
reg loop_ok=1'd0;
always@(negedge CLK_10M or negedge rst_n)
begin
	if(!rst_n)
	begin
		cnt<=16'd0;
		CS<=1'b1;
		start_16<=1'b0;
	end
	else begin 
		if(cnt<100)
		begin
			cnt<=cnt+1'd1;
			CS<=1'b1;
		end

		/*********唤醒IMU***************************/
		//wake up and reset
		else if(cnt<120)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h6b80;
		end
		else if(cnt<324)
		begin
			cnt<=cnt+1'b1;
			start_16<=1'b0;
			CS<=1'b1;
		end
		
		//turn off reset, pll clock
		else if(cnt<344)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h6b01;
		end
		else if(cnt<348)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//enable all sensors
		else if(cnt<368)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h6c00;
		end
		else if(cnt<372)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//1000HZ data output rate
		else if(cnt<392)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1900;
		end
		else if(cnt<396)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		/******低噪声模式，1000HZ输出******************************/
		//176Hz
		else if(cnt<416)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1A01;
		end
		else if(cnt<420)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//-250~250dps, use DLPF
		else if(cnt<440)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1B00;
		end
		else if(cnt<444)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//-4~4g
		else if(cnt<464)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1C08;
		end
		else if(cnt<468)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//218Hz
		else if(cnt<488)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1D01;
		end
		else if(cnt<492)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//low-power mode is disable
		else if(cnt<512)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h1D01;
		end
		else if(cnt<516)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		/*********允许数据采集完成中断*******************************/
		//int pin active high 50us
		else if(cnt<536)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h3710;
		end
		else if(cnt<540)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		//enable data ready interrupt
		else if(cnt<560)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h3801;
		end
		else if(cnt<564)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		/*********禁止I2C模式*********************/
		//disable I2C module and put the serial interface in SPI mode only
		else if(cnt<584)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h7040;
		end
		else if(cnt<590)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
//			CS<=1'b1;
		end
		
		
		/*******read data***************************/
		else if(cnt<609)		//Accelerometer x-axis H
		begin
			loop_ok<=1'd0;
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hbb00;
		end
		else if(cnt<610)
		begin
			CS<=1'b1;
			rddat0<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end

		else if(cnt<629)		//Accelerometer x-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hbc00;
		end
		else if(cnt<630)
		begin
			CS<=1'b1;
			rddat1<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<649)		//Accelerometer y-axis H
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hbd00;
		end
		else if(cnt<650)
		begin
			CS<=1'b1;
			rddat2<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<669)		//Accelerometer y-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hbe00;
		end
		else if(cnt<670)
		begin
			CS<=1'b1;
			rddat3<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<689)		//Accelerometer z-axis H
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hbf00;
		end
		else if(cnt<690)
		begin
			CS<=1'b1;
			rddat4<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<709)		//Accelerometer z-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc000;
		end
		else if(cnt<710)
		begin
			CS<=1'b1;
			rddat5<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<729)		//Gyroscope x-axis H
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc300;
		end
		else if(cnt<730)
		begin
			CS<=1'b1;
			rddat6<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<749)		//Gyroscope x-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc400;
		end
		else if(cnt<750)
		begin
			CS<=1'b1;
			rddat7<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<769)		//Gyroscope y-axis H
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc500;
		end
		else if(cnt<770)
		begin
			CS<=1'b1;
			rddat8<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<789)		//Gyroscope y-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc600;
		end
		else if(cnt<790)
		begin
			CS<=1'b1;
			rddat9<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		
		else if(cnt<809)		//Gyroscope z-axis H
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc700;
		end
		else if(cnt<810)
		begin
			CS<=1'b1;
			rddat10<=rddat_16;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
		else if(cnt<829)		//Gyroscope z-axis L
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'hc800;
		end
		else if(cnt<830)
		begin
			rddat11<=rddat_16;
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
//		
//		else if(cnt<898)		//Who I am
//		begin
//			CS<=1'b0;
//			cnt<=cnt+1'b1;
//			start_16<=1'd1;
//			wrdat_16<=16'hF500;
//		end
//		else if(cnt<902)
//		begin
//			rddat12<=rddat_16;
//			CS<=1'b1;
//			cnt<=cnt+1'b1;
//			start_16<=1'b0;
//		end
		
		else if(cnt<849)
		begin
			CS<=1'b0;
			cnt<=cnt+1'b1;
			start_16<=1'd1;
			wrdat_16<=16'h6b01;
		end
		else if(cnt<850)
		begin
			CS<=1'b1;
			cnt<=cnt+1'b1;
			start_16<=1'b0;
		end
		
      //loop		
		else if(cnt>=850)
		begin
			loop_ok<=1'd1;
		   cnt<=588;
	//		cnt<=320;
	//		cnt<=320;
		end
	end
end

///////////////////////////////////////////////////////////
reg [15:0] result0=16'd0/*synthesis noprune*/;
reg [15:0] result1=16'd0/*synthesis noprune*/;
reg [15:0] result2=16'd0/*synthesis noprune*/;
reg [15:0] result3=16'd0/*synthesis noprune*/;
reg [15:0] result4=16'd0/*synthesis noprune*/;
reg [15:0] result5=16'd0/*synthesis noprune*/;
always@(posedge loop_ok)
begin
	result0[15:8]=rddat0[7:0];
	result0[7:0]=rddat1[7:0];
	result1[15:8]=rddat2[7:0];
	result1[7:0]=rddat3[7:0];
	result2[15:8]=rddat4[7:0];
	result2[7:0]=rddat5[7:0];
	result3[15:8]=rddat6[7:0];
	result3[7:0]=rddat7[7:0];
	result4[15:8]=rddat8[7:0];
	result4[7:0]=rddat9[7:0];
	result5[15:8]=rddat10[7:0];
	result5[7:0]=rddat11[7:0];
end

endmodule