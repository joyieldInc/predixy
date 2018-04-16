## Custom Command
## CustomCommand {
##    command {                                      #command string, must be lowercase
##        [Mode read|write|admin[|[keyAt2|keyAt3]]   #default write, default key position is 1
##        [MinArgs [2-]]                             #default 2, including command itself
##        [MaxArgs [2-]]                             #default 2, must be MaxArgs >= MinArgs
##    }...
## }

## Currently support maximum 16 custom commands

## Example:
#CustomCommand {
##------------------------------------------------------------------------
#     custom.ttl {
#         Mode keyAt2
#         MinArgs 3
#         MaxArgs 3
#     }
####  custom.ttl miliseconds key
####  Mode = write|keyAt2, MinArgs/MaxArgs = 3 = command + miliseconds + key
##------------------------------------------------------------------------
## from redis source src/modules/hello.c
#     hello.push.native {
#         MinArgs 3
#         MaxArgs 3
#     }
####  hello.push.native key value
####  Mode = write, MinArgs/MaxArgs = 3 = command) + key + value
##------------------------------------------------------------------------
#     hello.repl2 {
#     }
####  hello.repl2 <list-key>
####  Mode = write, MinArgs/MaxArgs = 2 = command + list-key
##------------------------------------------------------------------------
#     hello.toggle.case {
#     }
####  hello.toggle.case key
####  Mode = write, MinArgs/MaxArgs = 2 = command + key
##------------------------------------------------------------------------
#     hello.more.expire {
#         MinArgs 3
#         MaxArgs 3
#     }
####  hello.more.expire key milliseconds
####  Mode = write, MinArgs/MaxArgs = 3 = command + key + milliseconds
##------------------------------------------------------------------------
#     hello.zsumrange {
#         MinArgs 4
#         MaxArgs 4
#         Mode read
#     }
####  hello.zsumrange key startscore endscore
####  Mode = read, MinArgs/MaxArgs = 4 = command + key + startscore + endscore
##------------------------------------------------------------------------
#     hello.lexrange {
#         MinArgs 6
#         MaxArgs 6
#         Mode read
#     }
####  hello.lexrange key min_lex max_lex min_age max_age
####  Mode = read, MinArgs/MaxArgs = 6 = command + key + min_lex + max_lex + min_age + max_age
##------------------------------------------------------------------------
#     hello.hcopy {
#         MinArgs 4
#         MaxArgs 4
#     }
#### hello.hcopy key srcfield dstfield
#### Mode = write, MinArgs/MaxArgs = 4 = command + key + srcfield) + dstfield
##------------------------------------------------------------------------
## from redis source src/modules/hellotype.c
#     hellotype.insert {
#         MinArgs 3
#         MaxArgs 3
#     }
####  hellotype.insert key value
####  Mode = write, MinArgs/MaxArgs = 3 = command + key + value
##------------------------------------------------------------------------
#     hellotype.range {
#         MinArgs 4
#         MaxArgs 4
#         Mode read
#     }
####  hellotype.range key first count
####  Mode = read, MinArgs/MaxArgs = 4 = command + key + first + count
##------------------------------------------------------------------------
#     hellotype.len {
#         Mode read
#     }
####  hellotype.len key
####  Mode = read, MinArgs/MaxArgs = 2 = command + key
##------------------------------------------------------------------------
#}

