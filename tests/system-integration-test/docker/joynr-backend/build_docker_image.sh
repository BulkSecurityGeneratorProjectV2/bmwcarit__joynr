#!/bin/bash

if [ -d target ]; then
	rm -Rf target
fi
mkdir target

for COPY_WAR in \
	../../../../java/backend-services/discovery-directory-servlet/target/discovery-directory-servlet.war \
	../../../../java/messaging/bounceproxy/single-bounceproxy/target/single-bounceproxy.war \
	../../../../java/backend-services/domain-access-controller-servlet/target/domain-access-controller-servlet.war
do
	if [ ! -f $COPY_WAR ]; then
		echo "Missing $COPY_WAR build artifact. Can't proceed."
		exit -1
	fi
	cp $COPY_WAR target/
done

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "The docker command seems to be unavailable."
	exit -1
fi

docker build -t joynr-backend:latest .
docker images | grep '<none' | awk '{print $3}' | xargs docker rmi -f 2>/dev/null
rm -Rf target
