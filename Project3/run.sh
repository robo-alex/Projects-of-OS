if lsmod | grep mypipe; then
    sudo rmmod mypipe
fi
make
sudo insmod mypipe.ko
sudo chmod 666 /dev/pipe_test
echo '\e[32;40;5;1m Generating file containing random data\e[m'
dd if=/dev/urandom of=test23M bs=23M count=1 iflag=fullblock

echo '\n\e[32;40;5;1m Communication Pipeline...\e[m'
dd if=test23M of=/dev/pipe_test bs=23M count=1 iflag=fullblock &
dd if=/dev/pipe_test of=recv23M bs=23M count=1 iflag=fullblock
wait

echo '\n\e[32;40;5;1m SHA256 hash check...\e[m'
sha256sum test23M | awk '{ print $1 }' > test23M_sha256
sha256sum recv23M | awk '{ print $1 }' > recv23M_sha256
echo "test23M: " $(cat test23M_sha256)
echo "recv23M: " $(cat recv23M_sha256)
if diff test23M_sha256 recv23M_sha256 -q; then
    echo "\n\e[32;40;5;1m Validation Success!\e[m"
else
    echo "\n\e[31;1m Validation Failed!\e[m"
fi

rm test23M recv23M test23M_sha256 recv23M_sha256
sudo rmmod mypipe