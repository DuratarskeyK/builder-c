# How to lint such project over PVS-studio

```docker build --tag=openmandriva/pvs --file Dockerfile.pvs .```

```docker run -ti --privileged=true -v /tmp/output:/output/ openmandriva/pvs:latest```

