function showGraph(gname, pfile, c);

% SHOWGRAPH
%   Graphical display of input graph,
%   and of the partitions of the graph.
%   The input files are assumed to have
%   the format used by the Chaco
%   partitioning software.
%
% Synopsis:
%   showGraph(gname, pfile, c)
%
% Input:
%   gname -- A text string, containing
%            the prefix of the graph input files.
%            Two input files are expected, one with the
%            suffix .graph, the other with the suffix .coords.
%   pfile -- A text string, containing the name of
%            a file containing partition assignments.
%            It is assumed that the file has the format
%            used by Chaco output files.
%            This argument is optional. If
%            it is _not_ present, showGraph will display
%            the original graph. If the pfile argument
%            _is_ present, the present function will read
%            the file and display the partitions. In that
%            case, the complete graph will be indicated in
%            the background.
%   c     -- A matlab color map. This argument is optional.
%            The default value is hsv(np), where np is the
%            number of partitions.
%
% The function does not return any value.
%
% Examples:
%  1. showGraph('hammond')
%     Plots the graph represented by the input files
%     hammond.graph and hammond.coords
%
%  2. showGraph('hammond','part.out')
%     Plots the partitions represented by the Chaco
%     output file part.out. In this case, the complete
%     graph is indicated by dotted lines in the background.
%
%  3. showGraph('hammond','part.out',myColors)
%     As example 2, but the color map myColors is used for
%     coloring the partitions.
%
% Authors: Jarmo Rantakokko and Michael Thune'
%          Dept. of Scientific Computing
%          Uppsala University, Sweden
%          E-mail: michael@tdb.uu.se
%
% Date: February 26, 1997

% Clear old graph

  clg;

% Create file names

  gfile = [gname '.graph'];
  cfile = [gname '.coords'];

% Read the graph coordinates

  fcoord = fopen(cfile,'r');
  G = fscanf(fcoord,'%e %e',[2, inf]);
  G = G';
  fclose(fcoord);

  [n, m] = size(G);

% Read the graph file with adjacency information

  A = sparse(n,n);

  fgraph = fopen(gfile,'r');

  skip = '%';
  while (skip == '%')
    ag = fgets(fgraph);
    skip = sscanf(ag,'%s',1);
  end

  for i = 1:n
    ag = fgets(fgraph);
    ac = sscanf(ag,'%u',inf);
    A(i,ac') = ones(1,length(ac));
  end

  fclose(fgraph);  

% Read partition assignments, and compute the number of graph nodes
  
  np = 0;             % Default assumption: no partitions.

  if nargin>1
    fpart = fopen(pfile,'r');
    part = fscanf(fpart,'%u',n);
    fclose(fpart);
    np = max(part)+1; % Number of partitions
  end

% Display the graph

  whitebg('w'); hold;

%...Set default colors, if necessary
  if nargin==2, c = hsv(np); end            

  if nargin==1
%...No partitions, display graph only
    gplot(A,G,'k-'); plot(G(:,1),G(:,2),'ko');
  else
%...Display the complete graph in the background
    gplot(A,G,'k:');

%...Display the partitions
    for i = 0:np-1
      pind = find(part==i);                       % Find the nodes
      ci = c(i+1,:);  
      plot(G(pind,1),G(pind,2),'o','Color',ci);   % Plot the nodes
      npi = length(pind);

      for j=1:npi                                 % Plot the edges
        jp = pind(j);
        this = G(jp,:);
        nbors=find(A(jp,pind));
        nbors=pind(nbors');
        nbors=nbors(nbors>jp);
        nnb = length(nbors);

        for k=1:nnb
         nbor = G(nbors(k),:);
         plot([this(1) nbor(1)],[this(2) nbor(2)], ...
              '-','Color',ci);
        end
      end
    end
  end
