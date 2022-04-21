import React from 'react'
import TimeAgoA from 'react-timeago'
// @ts-ignore
import zhCN from 'react-timeago/es6/language-strings/zh-CN'
// @ts-ignore
import buildFormatter from 'react-timeago/lib/formatters/buildFormatter'

const TimeAgoB = TimeAgoA as any

export const formatter = buildFormatter(zhCN)
export const formatterNoSuffix = (
  value: any,
  unit: any,
  _: any,
  epochMilliseconds: any,
  nextFormatter: () => any,
  now: () => any
) => formatter(value, unit, '', epochMilliseconds, nextFormatter, now)

const TimeAgo: typeof TimeAgoA = ((props: any) => <TimeAgoB formatter={formatter} component='span' {...props} />) as any
export const TimeAgoNoSuffix: typeof TimeAgoA = ((props: any) => <TimeAgoB formatter={formatterNoSuffix} component='span' {...props} />) as any

export default TimeAgo
