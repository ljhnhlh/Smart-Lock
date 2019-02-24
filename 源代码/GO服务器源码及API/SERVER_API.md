## API
---
#### Android_API
> GET localhost:3000/api/allUnlockingRecord

- return:
```json
{
    status: string //状态码
    msg: string //消息
    record: record[] //记录
}
//record结构
record{
    time: string //时间
}
```
* Time格式`YY:MM:DD HH:MM:SS`

> GET localhost:3000/api/WarningRecord

- return:
```json
{
    status: string //状态码
    msg: string //消息
    record: warningRecord[] //记录
}
//warningRecord
warningRecord{
    time: string //时间
    content: string //报警信息
}
```
* Time格式`YY:MM:DD HH:MM:SS`

> GET localhost:3000/api/LockState

- return:
```json
{
    status: string //状态码
    msg: string //消息
}
```
* `msg="门锁状态异常，请检查门锁"`
* `msg="门锁状态正常"`

> Post localhost:3000/api/Unlocking

- post body:
```json
{
    password: string //密码
}
```

- return:
```json
{
    status: string //状态码
    msg: string //消息
}
```

* 解锁成功：`msg="UnlockOK"`
* 解锁失败: `msg="UnlockFailed"`

---
