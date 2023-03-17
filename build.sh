#rm -rf build out;
docker run --rm -u $(id -u ${USER}):$(id -g ${USER}) -v $PWD:/app ghcr.io/bay0/switch-homebrew-template:latest;