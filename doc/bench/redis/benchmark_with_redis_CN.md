从我们的直观感受来讲，对于任何服务，只要在中间增加了一层，肯定会对服务性能造成影响。那么到底会影响什么呢？在考察一个服务性能的时候，有两个最重要的指标，那就是吞吐和延迟。吞吐定义为服务端单位时间内能处理的请求数，延迟定义为客户端从发出请求到收到请求的耗时。中间环节的引入我们首先想到的就是那会增加处理时间，这就会增加服务的延迟，于是顺便我们也会认为吞吐也会下降。从单个用户的角度来讲，事实确实如此，我完成一个请求的时间增加了，那么我单位时间内所能完成的请求量必定就减少了。然而站在服务端的角度来看，虽然单个请求的处理时间增加了，但是总的吞吐就一定会减少吗？

接下来我们就来对redis来进行一系列的测试，利用redis自带的redis-benchmark，分别对set和get命令；单个发送和批量发送；直连redis和连接redis代理[predixy](https://github.com/joyieldInc/predixy)。这样组合起来总共就是八种情况。redis-benchmark、redis是单线程的，predixy支持多线程，但是我们也只运行一个线程，这三个程序都运行在一台机器上。

|项目|内容|
|---|---|
|CPU|AMD Ryzen 7 1700X Eight-Core Processor 3.775GHz|
|内存|16GB DDR4 3000
|OS|x86_64 GNU/Linux 4.10.0-42-generic #46~16.04.1-Ubuntu
|redis|版本3.2.9，端口7200
|predixy|版本1.0.2，端口7600


八个测试命令

|测试命令|命令行|
|--|--|
|redis set|redis-benchmark -h xxx -p 7200 -t set -r 3000 -n 40000000
|predixy set|redis-benchmark -h xxx -p 7600 -t set -r 3000 -n 40000000
|redis get|redis-benchmark -h xxx -p 7200 -t get -r 3000 -n 40000000
|predixy get|redis-benchmark -h xxx -p 7600 -t get -r 3000 -n 40000000
|redis 批量set|redis-benchmark -h xxx -p 7200 -t set -r 3000 -n 180000000 -P 20
|predixy 批量set|redis-benchmark -h xxx -p 7600 -t set -r 3000 -n 180000000 -P 20
|redis 批量get|redis-benchmark -h xxx -p 7200 -t get -r 3000 -n 420000000 -P 20
|predixy 批量get|redis-benchmark -h xxx -p 7600 -t get -r 3000 -n 220000000 -P 20

以上8条命令采取redis-benchmark默认的50个并发连接，数据大小为2字节，指定3000个key，批量测试时一次发送20个请求。依次间隔2分钟执行以上命令，每一个测试完成时间大约4分钟。最后得到下图的总体结果：
![整体结果](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/overview.png)
眼花缭乱是不是？左边的纵轴表示CPU使用率，右边的纵轴表示吞吐。其中redis used表示redis总的CPU使用率，redis user表示redis CPU用户态使用率，redis sys表示redis CPU内核态使用率，其它类推。先别担心分不清里面的内容，下面我们会一一标出数值来。在这图中总共可以看出有八个凸起，依次对应我们上面提到的八个测试命令。

1 redis set测试
![redis_set](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/redis_set.png)

2 predixy set测试
![predixy_set](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/predixy_set.png)

3 redis get测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/redis_get.png)

4 predixy get测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/predixy_get.png)

5 redis 批量set测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/redis_pipeline_set.png)

6 predixy 批量set测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/predixy_pipeline_set.png)

7 redis 批量get测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/redis_pipeline_get.png)

8 predixy 批量get测试
![这里写图片描述](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/predixy_pipeline_get.png)

图片还是不方便看，我们总结为表格：

|测试\指标|redis used|redis user|redis sys|predixy used|predixy user|predixy sys|redis qps|predixy qps|
|--|--|--|--|--|--|--|--|--|
|redis set|0.990|0.247|0.744|0|0|0|167000|3|
|predixy set|0.475|0.313|0.162|0.986|0.252|0.734|174000|174000|
|redis get|0.922|0.180|0.742|0|0|0|163000|3|
|predixy get|0.298|0.195|0.104|0.988|0.247|0.741|172000|172000|
|redis批量set|1.006|0.796|0.21|0|0|0|782000|3|
|predixy批量set|0.998|0.940|0.058|0.796|0.539|0.256|724000|724000|
|redis批量get|1|0.688|0.312|0|0|0|1708000|3|
|predixy批量get|0.596|0.582|0.014|0.999|0.637|0.362|935000|935000|

看到前四个的结果如果感到惊讶不用怀疑是自己看错了或者是测试结果有问题，这个结果是无误的。根据这个结果，那么可以回答我们最初提出的疑问，增加了代理之后并不一定会降低服务整体的吞吐！虽然benchmark并不是我们的实际应用，但是redis的大部分应用场景都是这样的，并发的接受众多客户端的请求，处理然后返回。

为什么会是这样的结果，看到这个结果后我们肯定想知道原因，这好像跟我们的想象不太一样。要分析这个问题，我们还是从测试的数据来入手，首先看测试1的数据，redis的CPU使用率几乎已经达到了1，对于单线程程序来说，这意味着CPU已经跑满了，性能已经达到了极限，不可能再提高了，然而这时redis的吞吐却只有167000。测试2的redis吞吐都比它高，并且我们明显能看出测试2里redis的CPU使用率还不如测试1的高，测试2里redis CPU使用率只有0.475。为什么CPU使用率降低了吞吐反而却还高了呢？仔细对比一下两个测试的数据，可以发现在测试1里，redis的CPU大部分都花在了内核态，高达0.744，而用户态只有0.247，CPU运行在内核态时虽然我们不能称之为瞎忙活，但是却无助于提升程序的性能，只有CPU运行在用户态才可能提升我们的程序性能，相比测试1，测试2的redis用户态CPU使用率提高到了0.313，而内核态CPU则大幅下降至0.162。这也就解释了为什么测试2的吞吐比测试1还要高。当然了，我们还是要继续刨根问底，为什么测试2里经过一层代理predixy后，redis的CPU使用情况发生变化了呢？这是因为redis接受一个连接批量的发送命令过来处理，也就是redis里所谓的pipeline。而predixy正是利用这一特性，predixy与redis之间只有一个连接（大多数情况下），predixy在收到客户端的请求后，会将它们批量的通过这个连接发送给redis处理，这样一来就大大降低了redis用于网络IO操作的开销，而这一部分开销基本都是花费在内核态。

对比测试1和测试2，引入predixy不仅直接提高了吞吐，还带来一个好处，就是redis的CPU使用率只有一半不到了，这也就意味着如果我再把剩下的这一半CPU用起来还可以得到更高的吞吐，而如果没有predixy这样一层的话，测试1结果告诉我们redis的CPU利用率已经到头了，吞吐已经不可能再提高。

测试3和测试4说明的问题与测试1和测试2一样，如果我只做了这四个测试，那么看起来好像代理的引入完全有助于提升我们的吞吐嘛。正如上面所分析的那样，predixy提升吞吐的原因是因为采用了批量发送手段。那么如果客户端的使用场景就是批量发送命令，那结果会如何呢？

于是有了后面四个测试，后面四个测试给我们的直接感受就是太震撼了，吞吐直接提升几倍甚至10倍！其实也正是因为redis批量模式下性能非常强悍，才使得predixy在单命令情况下改进吞吐成为可能。当然到了批量模式，从测试结果看，predixy使得服务的吞吐下降了。

具体到批量set时，直连redis和通过predixy，redis的CPU使用率都满了，虽然采用predixy使得redis的用户态CPU从0.796提高到了0.940，但是吞吐却不升反降，从782000到724000，大约下降了7.4%。至于为什么用户态CPU利用率提高了吞吐却下降了，要想知道原因就需要分析redis本身的实现，这里我们就不做详细探讨。可以做一个粗糙的解释，在redis CPU跑满的情况下，不同的负载情况会使得用户态和内核态的使用率不同，而这其中有一种分配情况会是吞吐最大，而用户态使用率高于或者低于这种情况时都会出现吞吐下降的情况。

再来看批量get，直连redis时吞吐高达1708000，而通过predixy的话就只有935000了，下降了45%！就跟纳了个人所得税上限一般。看到这，刚刚对predixy建立的美好形象是不是又突然觉得要坍塌了？先别急，再看看其它指标，直连redis时，redis CPU跑满；而通过predixy时redis CPU只用了0.596，也就是说redis还有四成的CPU等待我们去压榨。

写到这，既然上面提到批量get时，通过predixy的话redis并未发挥出全部功力，于是就想着如果全部发挥出来会是什么情况呢？我们继续增加两个测试，既然单个predixy在批量的情况下造成了吞吐下降，但是给我们带来了一个好处是redis还可以提升的余地，那么我们就增加predixy的处理能力。因此我们把predixy改为三线程，再来跑一遍测试6和测试8。
两个测试的整体结果如下。
![mt_overview](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/mt_overview.png)

三线程predixy批量set
![mt_predixy_set](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/mt_predixy_pipeline_set.png)

三线程predixy批量get
![mt_predixy_get](https://github.com/joyieldInc/predixy/blob/master/doc/bench/redis/mt_predixy_pipeline_get.png)

|测试\指标|redis used|redis user|redis sys|predixy used|predixy user|predixy sys|redis qps|predixy qps|
|--|--|--|--|--|--|--|--|--|
|predixy pipeline set|1.01|0.93|0.07|1.37|0.97|0.41|762000|762000|
|predixy pipeline get|0.93|0.85|0.08|2.57|1.85|0.72|1718000|1718000|

原本在单线程predixy的批量set测试中，predixy和redis的CPU都已经跑满了，我们觉得吞吐已经达到了极限，但是实际结果显示在三线程predixy的批量set测试中，吞吐还是提高了，从原来的724000到现在的76200，与直连的782000只有2.5%的差距。多线程和单线程的主要差别在于单线程时predixy与redis只有一个连接，而三线程时有三个连接。

而对于三线程predixy的批量get测试，不出我们所料的吞吐得到了极大的提升，从之前的935000直接飙到1718000，已经超过了直连的1708000。

最后，我们来总结一下，我们整个测试的场景比较简单，只是单纯的set、get测试，并且数据大小为默认的2字节，实际的redis应用场景远比这复杂的多。但是测试结果的数据依旧可以给我们一些结论。代理的引入并不一定会降低服务的吞吐，实际上根据服务的负载情况，有时候引入代理反而可以提升整个服务的吞吐，如果我们不计较代理本身所消耗的资源，那么引入代理几乎总是一个好的选择。根据我们上面的分析，一个最简单实用的判断原则，看看你的redis CPU使用情况，如果花费了太多时间在内核态，那么考虑引入代理吧。
