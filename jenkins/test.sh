docker run -v $(pwd):/OpenStaDB openroad bash -c "OpenStaDB/test/regression fast && OpenStaDB/src/resizer/test/regression fast"
