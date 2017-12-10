==> search_pv_clk.cc <==
// 通过 pv log 和已经拼接上 query 的 searchlog 拼接 show 和 clk

==> search_pv_clk_r2.cc <==
// 把拼接好的 search log 转换成 原始 search log 的格式，即以 search_id 为 key, 按 rank 排序

==> search_pv_query_r1.cc <==
// 通过 url 对应的 query 出现次数，投票选出 search log 对应的 query, round 1

==> search_pv_query_r2.cc <==
// 通过 url 对应的 query 出现次数，投票选出 search log 对应的 query, round 2

==> search_pv_userid.cc <==
// 通过用户 id 拼接 search log query

==> to_train_data.cc <==
// 将拼接了 show clk 的原始数据转换为训练数据

==> url_query_count.cc <==
// 找出一个 url 命中的 query 以及命中次数
