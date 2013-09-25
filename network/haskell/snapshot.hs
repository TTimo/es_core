-- what does the stream of position updates looks like?
-- I need a type, object sequence (time can be inferred)
data Snapshot = Snapshot Int Int Double
              deriving (Show)
mySnap = Snapshot 0 10 1.0

-- think I'd rather straight up use time for now (the object's timeline)
-- also records are better etc.
data SnapshotRecord = SnapshotRecord {
  objectTime :: Int,
  objectPosition :: Double,
  proxyTime :: Int
  } deriving ( Show )
mySnap2 = SnapshotRecord {
  objectTime = 100,
  objectPosition = 1.1,
  proxyTime = 154 }

-- parametric type, we only have the times, and some other type is the position
data SnapshotParametric a = SnapshotParametric {
  param_objectTime :: Int,
  param_proxyTime :: Int,
  param_objectPosition :: a
  } deriving ( Show )
data DemoPosition = DemoPosition Double Double             
myParamSnap = SnapshotParametric DemoPosition {
  param_objectTime = 0,
  param_proxyTime = 102,
  param_objectPosition = DemoPosition 12.1 9.0 }
