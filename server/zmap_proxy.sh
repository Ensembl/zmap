
# environment hacked from otterlace_env.sh
# set up internet proxies so we have acceess

# Do not assume client is Inside.
#
# This will not re-configure for laptops which transition from guest
# to/from wired.
case "$( hostname -f 2>/dev/null || hostname )" in
    *.sanger.ac.uk)
        # On wired network.  Need proxy to fetch external resources,
        # but not to reach the Otter Server or a local Apache
        http_proxy=http://webcache.sanger.ac.uk:3128
        export http_proxy
        no_proxy=.sanger.ac.uk,localhost
        export no_proxy
        ;;
esac

# Copy the *_proxy variables we want into *_PROXY, to simplify logic
if [ -n "$http_proxy" ]; then
    HTTP_PROXY="$http_proxy"
    export HTTP_PROXY
else
    unset  HTTP_PROXY
fi
if [ -n "$no_proxy" ]; then
    NO_PROXY="$no_proxy"
    export NO_PROXY
else
    unset  NO_PROXY
fi

# Copy http_proxy to https_proxy
if [ -n "$http_proxy" ]; then
    https_proxy="$http_proxy"
    HTTPS_PROXY="$http_proxy"
    export https_proxy HTTPS_PROXY
else
    unset  https_proxy HTTPS_PROXY
fi

