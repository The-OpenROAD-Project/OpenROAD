docker build -f jenkins/Dockerfile.dev -t OpenStaDB .
docker run -v $(pwd):/OpenStaDB openroad bash -c "./OpenStaDB/jenkins/install.sh"
