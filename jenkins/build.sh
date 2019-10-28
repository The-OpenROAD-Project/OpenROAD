docker build -f jenkins/Dockerfile.dev -t OpenStaDB .
docker run -v $(pwd):/OpenStaDB opensta_db bash -c "./verilog2db/jenkins/install.sh"
