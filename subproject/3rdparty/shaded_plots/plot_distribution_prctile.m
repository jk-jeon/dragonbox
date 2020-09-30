function plot_distribution_prctile(varargin)
% PLOT_DISTRIBUTION_PRCTILE(X,Y) - Plots the median with percentile errors as a shaded region in the open 
% figure window.
% Inputs:
%     X: vector of n domain values (the x-axis).
%     Y: mxn matrix of distribution of range values (the y-axis), where m 
%           corresponds to the number of data samples for each value n.
%
% PLOT_DISTRIBUTION_PRCTILE(X,Y,...)
% Parameter options include:
%     'Alpha': the alpha value of the shaded region, default 0.15.
%     'Color': the shaded region color.
%     'LineWidth': the contour line width, default = 2.0.
%     'Prctile': the percentile values to plot, default [50] (the middle 
%           50 percentile, or inter-quartile range).
%


[X,Y,prctile_value,color_value,alpha_value,line_width] = parseinputs(varargin{:});

p_value = 0.5*(100-sort(prctile_value));

hold on
% Create the polygons for the shaded region
for j=1:numel(p_value)
    Ptop = prctile(Y,100-p_value(j));
    Pbot = prctile(Y,p_value(j));
    for i=1:numel(X)-1
        Px = [X(i) X(i+1) X(i+1) X(i)];
        Py = [Ptop(i) Ptop(i+1) Pbot(i+1) Pbot(i)];
        fill(Px,Py,color_value,'FaceAlpha',alpha_value,'EdgeColor','none');
    end
end
%plot(X,median(Y),'LineWidth',line_width,'Color',color_value);
%%%% This part is modified by Junekey Jeon %%%%
plot(X,median(Y),'--','LineWidth',line_width,'Color',color_value);
hold off


end





function [X,Y,prctile_value,color_value,alpha_value,line_width] = parseinputs(varargin)

% Check the number of input args
minargs = 2;
numopts = 4;
maxargs = minargs + 2*numopts;
narginchk(minargs,maxargs);

ax = gca;

% Set the defaults
prctile_value = 50;
alpha_value = 0.15;
color_value = ax.ColorOrder(ax.ColorOrderIndex,:);
line_width = 2.0;


% Get the inputs and check them
X = varargin{1};
validateattributes(X,{'numeric'},{'vector','nonnan','finite'},mfilename,'X',2);
Y = varargin{2};
validateattributes(Y,{'numeric'},{'2d','nonnan','finite','ncols',numel(X)},mfilename,'Y',2);


if nargin > minargs
    for i=3:2:nargin
        PNAME = varargin{i};
        PVALUE = varargin{i+1};
        
        PNAME = validatestring(PNAME,{'Alpha','Color','LineWidth','Prctile'},...
            mfilename,'ParameterName',i);
        
        switch PNAME
            case 'Alpha'
                validateattributes(PVALUE,{'numeric'},{'scalar','nonnan','finite','nonnegative','<=',1.0},mfilename,PNAME,i+1);
                alpha_value = PVALUE;
            case 'Color'
                validateattributes(PVALUE,{'numeric'},{'real','nonnegative','nonempty','vector','numel',3,'<=',1.0},mfilename,PNAME,i+1);
                color_value = PVALUE;
            case 'LineWidth'
                validateattributes(PVALUE,{'numeric'},{'scalar','finite','nonnegative'},...
                    mfilename,PNAME,i+1);
                line_width = PVALUE;
            case 'Prctile'
                validateattributes(PVALUE,{'numeric'},{'vector','finite','nonnan','nonnegative','<=',100},...
                    mfilename,PNAME,i+1);
                prctile_value = PVALUE;  
        end
    end
end

end

