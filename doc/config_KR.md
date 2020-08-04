# Predixy 구성 문서 설명

predixy 서비스를 정상적으로 실행하려면 구성 파일이 필수적이며 일반적인 predixy 서비스를 시작하려면 다음 명령을 실행하십시오.

    $ predixy <config-file> [--ArgName = ArgValue] ...

predixy는 먼저 config-file 파일에서 구성 정보를 읽습니다. 지정된 명령 행 매개 변수가 있으면 구성 파일에 정의 된 해당 값을 명령 행 매개 변수의 값으로 겹쳐 씁니다.

## 구성 파일 형식 설명
구성 파일은 줄 단위가있는 텍스트 파일이며 각 줄은 다음 유형 중 하나입니다.

+ 빈 줄 또는 주석
+ 키 값
+ 키 값 {}

### 규칙

+ #으로 시작하는 내용은 주석 내용입니다.
+ Include는 Value로 지정된 파일을 나타내는 특수 키이며, 절대 경로가 아닌 경우 상대 경로는 현재 파일이있는 경로입니다.
+ 값은 비어있을 수 있습니다. 값 자체에 #, 큰 따옴표가 포함 된 경우 큰 따옴표로 묶어야하며 큰 따옴표는 백 슬래시로 이스케이프 처리됩니다 (예 : "A \"# special # \ "value"
+ 여러 줄이 동일한 키를 정의하면 마지막 줄이 이전 정의를 덮어 씁니다.


## 기본 구성 부분

### Name

INFO 명령을 사용할 때 출력 될 predixy 서비스의 이름을 정의하십시오.

예:

    Name PredixyUserInfo

### Bind

predixy 서비스로 모니터링되는 주소를 정의하고 ip : port 및 unix 소켓을 바인딩 하십시오.

예:

    Bind 0.0.0.0:7617
    Bind /tmp/predixy

지정하지 않은 경우 : 0.0.0.0:7617

### WorkerThreads

작업자 스레드 수를 지정하십시오.

예:

    WorkerThreads 4

지정하지 않은 경우 : 1

### MaxMemory

단위 (G / M / K)로 지정할 수있는 predixy에 할당 할 수있는 최대 메모리를 지정하십시오. 0은 제한이 없음을 의미합니다

예:

    MaxMemory 1024000
    MaxMemory 1G

지정하지 않은 경우 : 0

### ClientTimeout

클라이언트 시간 초과 시간을 초 단위로 지정합니다. 즉, 유휴 시간이이 시간을 초과하면 클라이언트가 클라이언트와의 연결을 끊습니다.이 값이 0이면 기능이 비활성화되고 클라이언트와의 연결이 끊어지지 않습니다

예:

    ClientTimeout 300

지정하지 않은 경우 : 0

### BufSize

IO 버퍼 크기, predixy는 내부적으로 클라이언트 명령 및 서버 응답을 수신하기 위해 BufSize 크기의 버퍼를 할당하고 사본이없는 서버 나 클라이언트로 전달합니다. 값이 너무 작 으면 성능에 영향을 미치며 너무 크면 공간을 낭비 할 수 있고, 성능에 이점이 없습니다. 그러나 실제 응용 프로그램 시나리오에 따라 얼마나 적절한 지 파악해야 합니다. predixy는 기본적으로 값을 4096으로 설정합니다.

예:

    BufSize 8192

지정하지 않은 경우 : 4096

### 로그

로그 파일 이름을 지정하십시오.

예:

    로그 /var/log/predixy.log

지정하지 않으면 predixy의 동작은 표준 출력에 로그를 작성하는 것입니다.

### LogRotate

자동 로그 세그먼트 화 옵션은 시간, 파일 크기 또는 둘 다로 지정할 수 있습니다. 시간별 지정은 다음 형식을 지원합니다.

+ 1d 1일
+ nh 1 <= n <= 24 n시간
+ nm 1 <= n <= 1440 n분

파일 크기로 지정된 G 및 M 단위 지원

예:

    LogRotate 1d # 하루에 한 번 분할
    LogRotate 1h # 시간마다 한 번씩 분할
    LogRotate 10m # 10 분마다 한 번씩 분할
    LogRotate 2G # 로그 파일은 2G마다 나눔
    LogRotate 200M # 로그 파일은 200M마다 분할
    LogRotate 1d 2G #Split 하루에 한 번, 로그 파일이 2G에 도달하면 분할

정의되지 않은 경우 기능 비활성화

### LogXXXSample

로그 출력 샘플링 속도는이 레벨의 로그가 로그에 출력되는 횟수를 나타내며, 0이면이 레벨의 로그가 출력되지 않음을 의미합니다. 지원되는 수준은 다음과 같습니다.

+ LogVerbSample
+ LogDebugSample
+ LogInfoSample
+ LogNoticeSample
+ LogWarnSample
+ LogErrorSample

예:

    LogVerbSample 0
    LogDebugSample 0
    LogInfoSample 10000
    LogNoticeSample 1
    LogWarnSample 1
    LogErrorSample 1

이 매개 변수는 config set 명령을 통해 redis와 같이 온라인으로 수정할 수 있습니다.

    구성 설정 LogDebugSample 1

predixy에서 config 명령을 실행하려면 관리 권한이 필요합니다


## 액세스 제어 구성 섹션

predixy는 redis에서 AUTH 명령의 기능을 확장하고 여러 인증 비밀번호의 정의를 지원하며 각 비밀번호에 대한 권한을 지정할 수 있습니다 (권한은 읽기 권한, 쓰기 권한 및 관리 권한을 포함합니다) 쓰기 권한은 읽기 권한을 포함하며 관리 권한은 쓰기 권한을 포함합니다. 각 암호가 읽고 쓸 수있는 키 공간을 지정할 수도 있습니다. 키 공간의 정의는 키에 특정 접두사가 있음을 의미합니다.

권한 제어의 정의 형식은 다음과 같습니다.

    Authority {
        Auth [password] {
            Mode read|write|admin
            [KeyPredix Predix...]
            [ReadKeyPredix Predix...]
            [WriteKeyPredix Predix...]
        }...
    }


Authority에서 여러 Auth를 정의 할 수 있으며 각 Auth는 비밀번호를 지정하며 각 Auth에 대한 권한 및 키 공간을 정의 할 수 있습니다.

매개 변수 설명 :

+ 모드 : 지정해야하며 읽기, 쓰기, 관리 중 하나 일 수 있으며 각각 읽기, 쓰기 및 관리 권한을 나타냅니다.
+ KeyPrefix : [옵션], 키 공간을 정의 할 수 있으며 여러 키 공간은 공백으로 구분됩니다.
+ ReadKeyPrefix : [옵션], 읽을 수있는 키 공간을 정의 할 수 있으며 여러 키 공간은 공백으로 구분됩니다.
+ WriteKeyPrefix : [옵션], 쓰기 가능한 키 공간을 정의 할 수 있으며 여러 키 공간은 공백으로 구분됩니다.

읽을 수있는 키 공간의 경우 ReadKeyPrefix가 정의되면 ReadKeyPrefix에 의해 결정되고 그렇지 않으면 KeyPrefix에 의해 결정됩니다. 두 가지가 없으면 제한이 없습니다. 쓰기 가능한 지안 공간 해석에서도 마찬가지입니다. 쓰기 권한이 있다는 것은 읽기 권한이 있음을 의미하지만 읽기 및 쓰기 키 공간은 완전히 독립적입니다. 즉, WriteKeyPrefix는 기본적으로 ReadKeyPrefix의 내용을 포함하지 않습니다.

예:

    Authority {
        Auth {
            Mode read
            KeyPrefix Info
        }
        Auth readonly {
            Mode read
        }
        Auth modify {
            Mode write
            ReadPrefix User Stats
            WritePrefix User
        }
    }

위의 예는 3 개의 인증 비밀번호를 정의합니다.

+ 빈 암호. 암호가 비어 있으므로이 인증은 기본 인증이며 읽기 권한이 있습니다 .KeyPrefix가 지정되었으므로 최종 권한은 접두사가 Info 인 키만 읽을 수 있습니다.
+ 읽기 전용 비밀번호,이 인증에는 읽기 권한이 있으며 키 공간 제한이 없으며 모든 키를 읽을 수 있습니다
+ 비밀번호 수정,이 인증에는 쓰기 권한이 있으며 각각 읽을 수있는 키 공간 사용자와 통계를 정의하므로이 두 접두어로 키를 읽을 수 있으며 쓰기 가능한 키 공간은 사용자로 정의되므로 접두사 User로 쓸 수 있습니다. 키이지만 통계 접두사가있는 키는 쓸 수 없습니다

기본 사전 프록시 권한 제어는 다음과 같이 정의됩니다.

    Authority {
        Auth {
            Mode write
        }
        Auth "#a complex password#" {
            Mode admin
        }
    }

암호없이 모든 키를 읽고 쓸 수 있지만 관리 모드에는 암호 # 복잡한 암호 #가 필요합니다.

## Redis 인스턴스 구성 섹션

predixy는 Redis를 사용하기 위해 Redis Sentinel 및 Redis Cluster를 지원하며이 두 가지 형식 중 하나만 구성에 나타날 수 있습니다.

### Redis Sentinel 양식

정의 형식은 다음과 같습니다.

    SentinelServerPool {
        [Password xxx]
        [Databases number]
        Hash atol|crc16
        [HashTag "xx"]
        Distribution modula|random
        [MasterReadPriority [0-100]]
        [StaticSlaveReadPriority [0-100]]
        [DynamicSlaveReadPriority [0-100]]
        [RefreshInterval number[s|ms|us]]
        [ServerTimeout number[s|ms|us]]
        [ServerFailureLimit number]
        [ServerRetryTimeout number[s|ms|us]]
        [KeepAlive seconds]
        Sentinels {
            + addr
            ...
        }
        Group xxx {
            [+ addr]
            ...
        }
    }

매개 변수 설명 :

+ 비밀번호 : redis 인스턴스에 연결하기위한 기본 비밀번호를 지정합니다. 지정하지 않으면 redis에 비밀번호가 필요하지 않습니다.
+ 데이터베이스 : redis db 수를 지정하십시오 (지정하지 않은 경우 1 임).
+ 해시 : 키 해시 방법을 지정합니다. 현재 atol 및 crc16 만 지원됩니다.
+ HashTag : 지정되지 않은 경우 해시 태그 지정 {}
+ 분포 : 키 분포 방법을 지정합니다. 현재 모듈과 랜덤 만 지원됩니다.
+ MasterReadPriority : Redis 마스터 노드에서 읽기 요청을 실행하는 우선 순위 인 읽기 및 쓰기 분리 기능, 0 인 경우 redis 마스터를 읽을 수 없습니다 (지정하지 않은 경우 50).
+ StaticSlaveReadPriority : 정적 redis 슬레이브 노드의 읽기 요청 우선 순위 인 읽기 및 쓰기 분리 기능 소위 정적 노드는 이 구성 파일에 나열된 redis 노드를 나타내며, 지정하지 않으면 0입니다.
+ DynamicSlaveReadPolicy : 위 함수 참조. 소위 동적 노드는이 구성 파일에 나열되지 않지만 redis sentinel을 통해 동적으로 감지되는 노드를 나타냅니다.
+ RefreshInterval : predixy는 최신 클러스터 정보를 얻기 위해 정기적으로 redis sentinel을 요청합니다. 이 매개 변수는 새로 고침 간격을 초 단위로 지정하거나 지정하지 않으면 1 초를 지정합니다.
+ ServerTimeout : predixy에서 가장 긴 요청 처리 / 대기 시간이 시간 후에 redis가 응답하지 않으면 predixy는 redis와의 연결을 닫고 클라이언트에게 오류 응답을 제공합니다. 이 옵션은 작동하지 않습니다. 0이면이 기능이 비활성화됩니다. 즉, redis가 반환되지 않으면 영원히 기다립니다. 지정하지 않으면 0이됩니다.
+ ServerFailureLimit : 오류가 유효하지 않은 것으로 표시되기 전에 redis 인스턴스가 몇 번 나타나거나 지정되지 않은 경우 10
+ ServerRetryTimeout : Redis 인스턴스가 정상으로 돌아 왔는지 확인하지 못한 후 (지정하지 않은 경우 1 초)
+ KeepAlive : predixy와 redis 사이의 연결에 대한 tcp keepalive 시간, 0은 지정되지 않은 경우이 기능을 비활성화합니다. 0
+ 센티넬 : 레디 스 센티넬 인스턴스의 주소 정의
+ 그룹 : redis 그룹 정의 그룹 이름은 redis sentinel의 이름과 같아야하며 redis 주소는 그룹에 표시 될 수 있으며 목록은 위에서 언급 한 정적 노드입니다.

한 가지 예 :

    SentinelServerPool {
        Databases 16
        Hash crc16
        HashTag "{}"
        Distribution modula
        MasterReadPriority 60
        StaticSlaveReadPriority 50
        DynamicSlaveReadPriority 50
        RefreshInterval 1
        ServerTimeout 1
        ServerFailureLimit 10
        ServerRetryTimeout 1
        KeepAlive 120
        Sentinels {
            + 10.2.2.2:7500
            + 10.2.2.3:7500
            + 10.2.2.4:7500
        }
        Group shard001 {
        }
        Group shard002 {
        }
    }

  이 Redis Sentinel 클러스터 정의는 3 개의 redis 센티넬 인스턴스, 즉 10.2.2.2:7500, 10.2.2.3:7500, 10.2.2.4:7500을 지정하고 shard001 및 shard002라는 두 그룹의 redis를 정의합니다. 
  
  정적 redis 노드가 지정되지 않았습니다. 모든 redis 인스턴스는 비밀번호 인증을 사용하지 않으며 모두 16 db입니다. 
  
  Predixy는 crc16을 사용하여 키의 해시 값을 계산 한 다음 모듈러스 방법 인 모듈러스를 통해 키를 shard001 또는 shard002에 배포합니다. 
  
  MasterReadPriority가 60 (DynamicSlaveReadPriority의 50보다 큼)이므로 읽기 요청은 redis 마스터 노드에 분배되고 RefreshInterval은 1이며, 1 초마다 클러스터 정보를 새로 고치기 위해 redis sentinel에 요청을 보냅니다. 
  
  redis 인스턴스가 10 회 실패한 후 redis 인스턴스는 유효하지 않은 것으로 표시되며 1 초마다 복원되는지 확인합니다.

### Redis 클러스터 양식

정의 형식은 다음과 같습니다.

    ClusterServerPool {
        [Password xxx]
        [MasterReadPriority [0-100]]
        [StaticSlaveReadPriority [0-100]]
        [DynamicSlaveReadPriority [0-100]]
        [RefreshInterval seconds]
        [ServerTimeout number[s|ms|us]]
        [ServerFailureLimit number]
        [ServerRetryTimeout number[s|ms|us]]
        [KeepAlive seconds]
        Servers {
            + addr
            ...
        }
    }


매개 변수 설명 :

+ 선택적 파라미터는 Redis Sentinel 모드에서 동일한 이름과 동일한 의미를 갖습니다.
+ 서버 : 여기에 redis 클러스터에 redis 인스턴스를 나열합니다. 여기에 나열된 노드는 정적 노드이고, 나열되지 않았지만 클러스터 정보를 통해 찾은 인스턴스는 동적 노드입니다.

한 가지 예 :

    ClusterServerPool {
        MasterReadPriority 0
        StaticSlaveReadPriority 50
        DynamicSlaveReadPriority 50
        RefreshInterval 1
        ServerTimeout 1
        ServerFailureLimit 10
        ServerRetryTimeout 1
        KeepAlive 120
        Servers {
            + 192.168.2.107:2211
            + 192.168.2.107:2212
        }
    }

정의는 클러스터 정보가 redis 인스턴스 192.168.2.107:2211 및 192.168.2.107:2212를 통해 발견되고 MasterReadPriority가 0으로 지정되도록 지정합니다. 즉, 읽기 요청을 redis 마스터 노드에 분배하지 않습니다.


## 다중 데이터 센터 구성 섹션

Predixy는 여러 데이터 센터를 지원하며, 데이터 센터에 redis를 배포하면 데이터 센터의 redis 인스턴스에 읽기 요청을 분산시켜 크로스 데이터 센터 액세스를 피할 수 있습니다. 실제로 데이터 센터 개념을 적용하면 실제로 여러 데이터 센터가없는 경우에도이 구성을 사용하여 노드의 읽기 요청을 공유해야 할 때 요청 분배를 제어 할 수 있습니다. 예를 들어 현재 랙은 데이터 센터로 간주 될 수 있습니다.

다중 데이터 센터 구성 형식 :

    LocalDC name
    DataCenter {
    DC name {
        AddrPrefix {
            + IpPrefix
            ...
        }
        ReadPolicy {
            name priority [weight]
            other priority [weight]
        }
    }
    ...
    }


매개 변수 설명 :

+ LocalDC : predixy가 현재 위치한 데이터 센터를 지정합니다
+ DC : 데이터 센터 정의
+ AddrPrefix : 데이터 센터에 포함 된 IP 접두사 정의
+ ReadPolicy :이 데이터 센터에서 다른 (자신의 데이터 센터 포함) 데이터 센터를 읽는 우선 순위 및 가중치 정의

데이터 센터 기능을 사용하지 않으면 LocalDC 및 DataCenter 정의를 제공하지 않습니다.

여러 데이터 센터 구성의 예 :

    DataCenter {
        DC bj {
            AddrPrefix {
                + 10.1
            }
            ReadPolicy {
                bj 50
                sh 20
                sz 10
            }
        }
        DC sh {
            AddrPrefix {
                + 10.2
            }
            ReadPolicy {
                sh 50
                bj 20 5
                sz 20 2
            }
        }
        DC sz {
            AddrPrefix {
                + 10.3
            }
            ReadPolicy {
                sz 50
                sh 20
                bj 10
            }
        }
    }

이 예는 3 개의 데이터 센터, 즉 bj, sh 및 sz를 정의합니다. bj 데이터 센터에는 ip 접두사가 10.1 인 redis 인스턴스가 포함되므로 predixy가 redis sentinel 또는 redis 클러스터를 통해 노드를 발견하면 redis 인스턴스의 주소 접 두부가 10.1 인 경우 인스턴스가 bj 데이터 센터에있는 것으로 간주됩니다. predixy는 자체 데이터 센터에 따라 해당 읽기 요청 전략을 선택합니다.

predixy가 bj 데이터 센터에 있다고 가정 할 때 bj를 읽는 bj의 우선 순위는 50이고, 다른 두 개보다 높습니다. 따라서 predixy는 읽기 조작을 위해 bj 데이터 센터에 우선 순위를 부여합니다 .bj 데이터 센터에 사용 가능한 redis 노드가없는 경우 sh 데이터 센터가됩니다. sh에 노드가 없으면 sz가 선택됩니다.

predixy가 sh 데이터 센터에 있다고 가정하고 predixy는 sh 데이터 센터를 선호합니다. sh 데이터 센터에 사용 가능한 redis 인스턴스가 없으면 bj 및 sh의 우선 순위가 모두 20이므로 트래픽은 가중치 설정에 따라 할당됩니다 (여기서는 5 부). 요청은 bj 데이터 센터로 이동하고 2 개의 요청은 sz로 이동합니다.

클러스터를 정의 할 때 마스터 및 슬레이브 노드의 읽기 우선 순위를 정의 할 수 있다고 말했지만 데이터 센터에는 읽기 우선 순위의 개념이 있으므로 어떻게 작동합니까? 원칙은 데이터 센터 기능을 활성화 한 후 데이터 센터 읽기 전략에 따라 데이터 센터를 선택한 다음 클러스터의 마스터-슬레이브 읽기 우선 순위를 사용하여 데이터 센터에서 최종 redis 인스턴스를 선택하는 것입니다.


## 지연 모니터링 구성 섹션

Predixy는 predixy가 요청을 처리하는 시간을 기록 할 수있는 강력한 지연 모니터링 기능을 제공하며 predixy의 경우 실제로 redis를 요청하는 시간입니다.

지연 모니터링의 정의 형식은 다음과 같습니다.

    LatencyMonitor name {
        Commands {
            + cmd
            [- cmd]
            ...
        }
        TimeSpan {
            + TimeElapsedUS
            ...
        }
    }

매개 변수 설명 :

+ LatencyMonitor : 대기 시간 모니터 정의
+ 명령 : 지연 모니터링에 의해 기록되는 redis 명령을 지정하고, + cmd는 명령을 모니터링하는 것을 의미하고, -cmd는 명령을 모니터링하지 않는 것을 의미하며, cmd가 모두 인 경우 모든 명령을 의미합니다.
+ TimeSpan : 지연 버킷을 정의합니다 (마이크로 초). 이는 엄격하게 증가하는 시퀀스 여야합니다.

여러 LatencyMonitor를 정의하여 다른 명령을 모니터링 할 수 있습니다.

지연 모니터링 구성 예 :

    LatencyMonitor all {
        Commands {
            + all
            - blpop
            - brpop
            - brpoplpush
        }
        TimeSpan {
            + 1000
            + 1200
            + 1400
            + 1600
            + 1700
            + 1800
            + 2000
            + 2500
            + 3000
            + 3500
            + 4000
            + 4500
            + 5000
            + 6000
            + 7000
            + 8000
            + 9000
            + 10000
        }
    }

    LatencyMonitor get {
        Commands {
            + get
        }
        TimeSpan {
            + 100
            + 200
            + 300
            + 400
            + 500
            + 600
            + 700
            + 800
            + 900
            + 1000
        }
    }

    LatencyMonitor set {
        Commands {
            + set
            + setnx
            + setex
        }
        TimeSpan {
            + 100
            + 200
            + 300
            + 400
            + 500
            + 600
            + 700
            + 800
            + 900
            + 1000
        }
    }


위의 예제는 세 가지 지연 모니터를 정의합니다.이 모니터는 blpop / brpop / brpoplpush를 제외한 모든 명령을 모니터하고, get 모니터는 명령을 가져오고, set 모니터는 set / setnx / setex 명령을 모니터합니다. 여기에서 시간이 많이 소요되는 버킷의 정의가 모든 상황에 적용되는 것은 아니며 실제 사용시 실제 시간이 소요되는 조건을보다 정확하게 반영하도록 조정해야합니다.

INFO 명령을 통해 시간이 많이 걸리는 모니터링을 보려면 세 가지 방법이 있습니다.

### 모든 지연 정의의 전체 정보를 봅니다.

명령:

    redis> INFO

이 시점에 표시되는 결과는 전체 지연 정보입니다.

### 단일 지연 정의 정보보기

명령:

    redis> INFO Latency <name>

\<name>이 지연 정의의 이름 인 경우, 결과는 정의의 전체 지연 정보와 서브 백엔드 Redis 인스턴스의 지연 정보를 표시합니다.

백엔드 Redis 인스턴스의 지연 정보보기
명령:

    redis> INFO ServerLatency <serv> [name]

\<serv>는 redis 인스턴스의 주소이고, [name]은 선택적 지연 정의 이름입니다. 이름을 생략하면 redis 인스턴스에 요청 된 모든 지연 정의 정보가 제공됩니다. 그렇지 않으면 이름 만 제공됩니다.

지연 정보 형식 설명, 다음은 지연 정의 all의 전체 정보 예입니다.

    LatencyMonitorName : all
    <= 100 3769836 91339 91.34 %
    <= 200 777185 5900 97.24 %
    <= 300 287565 1181 98.42 %
    <= 400 1858913798.96 %
    <= 500 132773 299 99.26 %
    <= 600 85050156 99.41 %
    <= 700 85455133 99.54 %
    <= 800 40088 54 99.60 %
    <= 1000 67788 77 99.68 %
    > 1000 601012325 100.00 %
    T 60 6032643 100001

+ 첫 번째 열이 <= 인 경우 두 번째 열은 소요 시간보다 작거나 같음을 의미하고, 세 번째 열은이 범위에서 소비하는 시간의 합을 의미하고, 네 번째 열은 요청 수를 의미하고, 다섯 번째 열은 총 요청에서 누적 된 요청의 백분율을 의미합니다.
+ 첫 번째 열은>이고 두 번째 열은 시간이 오래 걸리는 요청을 나타내며 마지막 두 열은 위와 동일한 의미를 갖습니다. 이 행은 최대 한 번만 나타납니다.이 행에 요청이 너무 많으면 지연 정의로 지정된 시간이 많이 소요되는 버킷이 적합하지 않은 것입니다.
+ 첫 번째 열은 T이며,이 행은 한 번만 나타나며 항상 끝에 나타납니다. 두 번째 열은 모든 요청의 평균 시간을 나타내고, 세 번째 열은 총 요청 시간의 합계를 나타내고, 네 번째 열은 총 요청 수를 나타냅니다.