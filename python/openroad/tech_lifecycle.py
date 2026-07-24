from openroad import Tech, set_thread_count, thread_count

tech = Tech()
set_thread_count(2)
assert thread_count() == 2
