#!/bin/bash
if [ -n "$http_proxy" ]; then 
    if [[ "$http_proxy" =~ .*"@" ]]; then
        read protocol proxy_user proxy_password proxy_host proxy_port <<< $( echo ${http_proxy} | awk -F '://|:|@' '{ print $1, $2, $3, $4, $5 }')
    else
        read protocol proxy_host proxy_port <<< $( echo ${http_proxy} | awk -F'://|:' '{ print $1, $2, $3 }')
    fi
    echo "protocol = $protocol" 
    echo "proxy_host = $proxy_host"
    echo "proxy_port = $proxy_port"
    echo "proxy_user = $proxy_user"
    echo "proxy_password = $proxy_password"
fi

if [ -z "$proxy_host" ]
then
    echo "No proxy configured, using direct internet access."
    exit 0
else
    if [ -z "$proxy_port" ]
    then
        echo "No port for proxy defined."
        exit 1
    fi
    if [ -n "$proxy_user" ]
    then
        echo "Using authenticated proxy."
        if [ -z "$proxy_password" ]
        then
            echo "Proxy user is set but no proxy password supplied."
            exit 1
        fi
    else
        echo "Using Proxy."
    fi
fi
echo "Setting up proxy configuration in /etc/dnf/dnf.conf"
cat >> /etc/dnf/dnf.conf <<EOF
[main]
zchunk=false
gpgcheck=1
installonly_limit=3
clean_requirements_on_remove=false
proxy=$proxy
sslverify=false
EOF
echo "Final Configuration /etc/dnf/dnf.conf:"
cat /etc/dnf/dnf.conf
echo "Setting up proxy configuration in /etc/wgetrc"
cat > /etc/wgetrc <<EOF
use_proxy=on
http_proxy=$http_proxy
https_proxy=$http_proxy
ftp_proxy=$http_proxy
check_certificate=off
EOF
echo "Final Configuration /etc/wgetrc:"
cat /etc/wgetrc
echo "Setting up insecure curl configuration in /etc/.curlrc"
cat > /etc/.curlrc << EOF
insecure
EOF
echo "Setting up proxy configuration in /etc/profile.d/use-my-proxy.sh"
cat > /etc/profile.d/use-my-proxy.sh <<EOF
echo "use-my-proxy.sh started"
PROXY_HOST=$proxy_host
PROXY_PORT=$proxy_port
http_proxy="$http_proxy"
https_proxy="$http_proxy"
ftp_proxy="$http_proxy"
HTTP_PROXY="$http_proxy"
HTTPS_PROXY="$http_proxy"
FTP_PROXY="$http_proxy"
export PROXY_HOST
export PROXY_PORT
export http_proxy
export https_proxy
export ftp_proxy
export HTTP_PROXY
export HTTPS_PROXY
export FTP_PROXY
CURL_HOME=/etc
export CURL_HOME
echo "Setting up npm configuration in \$HOME/.npmrc"
cat > \$HOME/.npmrc << EOF2
strict-ssl=false
registry=http://registry.npmjs.org/
EOF2
MAVEN_OPTS="-Dmaven.wagon.http.ssl.insecure=true -Dmaven.wagon.http.ssl.allowall=true -Dmaven.wagon.http.ssl.ignore.validity.dates=true -Xms2048m -Xmx2048m"
export MAVEN_OPTS
echo "use-my-proxy.sh finished"
EOF
echo "Proxy configuration finished."
exit 0
