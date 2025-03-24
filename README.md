# morph-take-home

## Step 1
Clone https://github.com/fsouza/fake-gcs-server 

## Step 2

run ```bash ./utils/rand_files_gen.sh /path/to/fake-gcs-server/examples/data/my_backet```

It will create 10 files with random data at
`/path/to/fake-gcs-server/examples/data/my_backet` for testing later
## Step 3 

Start the fake GCS server with: 

```
docker run -d --name fake-gcs-server -p 4443:4443 -v ${PWD}/examples/data:/data fsouza/fake-gcs-server -scheme http
```

## Step 4

Build & start the caching layer.

```
mkdir build && cd build
cmake ..
make -j

./gcs_cache --cfg=../default.cfg  > my_log.txt 2>&1 
```

## Step 5 (in a separate terminal)

Simulate the load for the caching layer, pick one of the scenarios
```
cd $repo/test
python3 simulate_load.py -s noisy|polite|both
```
