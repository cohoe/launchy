Building
========

Fedora
------
This will build Launchy in a Fedora Docker container. You can then take the RPMs
generated and upload them to your repo of choice.

1) Update ```launchy.spec``` with the appropriate build/release info. Don't 
forget the changelog too!

2) Specify the correct version in the ```Dockerfile```.

3) Build the image for the specific version of Fedora.
    ```bash
    sudo docker build --tag launchy:fc28 .
    ```

4) Launch an instance
    ```bash
    sudo docker run --name launchy-fc28 -it launchy:fc28 /bin/bash
    ```

5) Run the build
    ```bash
    make build
    ```

6) Do something with the RPMs
    @TODO Someday this will be automated.
    ```
    [root@f71523fae13a launchy]# ls -l /root/rpmbuild/RPMS/x86_64/
    total 2684
    -rw-r--r--. 1 root root  569484 May  1 19:24 launchy-2.6-05.fc28.x86_64.rpm
    -rw-r--r--. 1 root root 2033496 May  1 19:24 launchy-debuginfo-2.6-05.fc28.x86_64.rpm
    -rw-r--r--. 1 root root  119028 May  1 19:24 launchy-debugsource-2.6-05.fc28.x86_64.rpm
    -rw-r--r--. 1 root root   15200 May  1 19:24 launchy-devel-2.6-05.fc28.x86_64.rpm
    [root@f71523fae13a launchy]# scp -r /root/rpmbuild/RPMS/x86_64/* gcohoe@172.17.0.1:/tmp/
    ```