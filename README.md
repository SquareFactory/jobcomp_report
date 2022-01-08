# jobcomp/report: A job completion plugin for Slurm using TRES Billing

**Warning:** The plugin was made for `slurm-21-08-3-1`.

## Configuring

Availables options are shown in the `CMakeLists.txt`.

## Compiling

Fetch the externals:

```sh
git clone --recurse-submodules -j$(nproc) <this repo>
```

If you forgot to put `recurse-submodules`:

```sh
git submodule update --init
```

Next, build the library. The outputs will be in the `build/lib` directory.

```sh
mkdir -p build
cd build && cmake ..
make -j$(nproc)
# Or with Ninja:
# cd build && cmake .. -G Ninja
# ninja
```

## Installing

```sh
sudo make install  # or sudo ninja install
```

This will install `jobcomp_report.a` and `jobcomp_report.so` in the `/usr/lib/slurm` directory.

To change the directory use:

```sh
mkdir -p build
cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/lib/slurm
make -j$(nproc)
sudo make install
```

## Using

In the slurm.conf of the slurm controller.

```conf
JobCompType=jobcomp/report
JobCompLoc=https://endpoint
```

When a job is `COMPLETING`, the plugin will POST to `JobCompLoc` with the format:

```yml
{
    "job_id": 1288,
    "user_id": 1611,
    "cluster": "reindeerpizza",
    "partition": "main",
    "state": "COMPLETING",
    "allocated_ressources": {
        "cpu": 1,
        "mem": 7000,
        "gpu": 0
    },
    "time_start": 1641583295,
    "job_duration": 188,
    "cost_tier": {
        "name": "normal",
        "facotr": 1
    },
    "total_cost": 13
}
```

The total cost calculation is `round((billing * elapsed * qos_usage_factor)/60.0)`.
Billing is an integer value and fetched in the `job_ptr->tres_alloc_cnt[TRES_ARRAY_BILLING]`.

## Maintaining and updating the plugin for new Slurm Version

The best way to maintain the plugin is to use the actual examples from Slurm.

First update the slurm dependency by using `git checkout`:

```sh
git submodule update --remote # Fetch latest commit
cd externals/slurm
git checkout slurm-21-08-4-1  # for example
# git checkout main  # for the latest slurm commit
```

Then, you can verify `slurm/slurm/plugins/jobcomp/none/jobcomp_none.c` which was the template for this plugin.

Other examples are in the `slurm/slurm/plugins/jobcomp` directory.

## LICENSE

```sh
MIT License

Copyright (c) 2021 Marc Nguyen, Cohesive Computing SA and DeepSquare Association

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```
