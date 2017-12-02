FAT 数据结构
struct msdos_inode_info {
spinlock_t cache_lru_lock;
struct list_head cache_lru;
int nr_caches;
/* for avoiding the race between fat_free() and fat_get_cluster() */
unsigned int cache_valid_id;

/* NOTE: mmu_private is 64bits, so must hold ->i_mutex to access */
loff_t mmu_private; /* physically allocated size */

int i_start; /* first cluster or 0 Â Â Â Â Â æ–‡ä»¶å¼€å§‹çš„ç‰©ç†ç°‡å·*/
int i_logstart; /* logical first cluster */
int i_attrs; /* unused attribute bits */
loff_t i_pos; /* on-disk position of directory entry or 0 *///æ–‡ä»¶ç›®å½•é¡¹ç´¢å¼•entryï¼Œå¯ä»¥ä½œä¸ºæ–‡ä»¶ç´¢å¼•èŠ‚ç‚¹ç”¨
struct hlist_node i_fat_hash; /* hash by i_location ï½†ï½ï½”è¡¨*/
struct hlist_node i_dir_hash; /* hash by i_logstart */
struct rw_semaphore truncate_lock; /* protect bmap against truncate */
struct inode vfs_inode; Â  Â  Â  Â  Â //åŸºç¡€çš„ï½‰ï½Žï½ï½„ï½…ç»“æž„
};
struct inode {
umode_t Â Â Â Â Â  i_mode;
unsigned short Â Â Â Â Â  i_opflags;
kuid_t Â Â Â Â Â  i_uid;ã€€ã€€kgid_t Â Â Â Â Â  i_gid;
unsigned intÂ Â Â Â Â  i_flags;

const struct inode_operations *i_op; Â  Â  ï¼ï¼ï½‰ï½Žï½ï½„ï½…æ“ä½œå‡½æ•°é›†
struct super_block *i_sb; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  ï¼ï¼æ‰€å±žçš„è¶…çº§å—
struct address_space *i_mapping; Â  Â  Â  Â  Â ï¼ï¼
Â Â Â Â Â //inodeç´¢å¼•èŠ‚ç‚¹å·ï¼Œ0å’Œ1é¢„ç•™ï¼ŒåŽé¢çš„fat_build_inode()ä¸­ç®—æ³•äº§ç”Ÿï¼Œinode->i_ino = iunique(sb, MSDOS_ROOT_INO);
unsigned long i_ino;Â

union {Â  Â Â  const unsigned int i_nlink;Â  Â Â  unsigned int __i_nlink;};ã€€ï¼ï¼æ–‡ä»¶å¯¹åº”ç¡¬é“¾æŽ¥æ•°ç›®
dev_t i_rdev; Â  Â  Â  Â  Â  Â  Â  Â  Â Â  Â Â Â Â Â ï¼ï¼è®¾å¤‡å·
loff_t i_size; Â  Â  Â  Â  Â  Â  Â  Â Â Â Â Â //æ–‡ä»¶å¤§å°å­—èŠ‚æ•°
struct timespec i_atime;ã€€struct timespec i_mtime;ã€€struct timespec i_ctime;
spinlock_t Â Â Â Â Â  i_lock; Â Â Â Â Â /* i_blocks, i_bytes, maybe i_size */
unsigned short i_bytes;ã€€ï¼ï¼æ–‡ä»¶æœ€åŽä¸€ä¸ªå—çš„å­—èŠ‚æ•°
unsigned int i_blkbits; Â  Â  ï¼ï¼ä¸€ä¸ªå—å¤šå°‘ï½‚ï½‰ï½”ï¼Œï¼•ï¼‘ï¼’å¯¹åº”ï¼™ï½‚ï½‰ï½”ï½“
blkcnt_t i_blocks; Â  Â  //æ–‡ä»¶å ç”¨çš„å®žé™…çš„æ‰‡åŒºå—çš„ä¸ªæ•°

unsigned long i_state;ã€€ã€€ã€€
struct mutex i_mutex;

unsigned long dirtied_when; /* jiffies of first dirtying */

struct hlist_node i_hash; Â  Â  //ç”¨æ¥ç²˜ç»“åˆ°hashè¡¨ä¸­æŸä¸ªèŠ‚ç‚¹ï¼Œè¿™ä¸ªèŠ‚ç‚¹é€šè¿‡inodeç´¢å¼•èŠ‚ç‚¹å·å¾—åˆ°ï¼Œå®žçŽ°å¿«é€Ÿç´¢å¼•
struct list_head i_wb_list; /* backing dev IO list */
struct list_head i_lru; /* inode LRU list */
struct list_head i_sb_list;
union {
struct hlist_head i_dentry;
struct rcu_head i_rcu;
};
u64 i_version; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â //æ–‡ä»¶ç‰ˆæœ¬å·ï¼Œåº”è¯¥æ˜¯æ¯æ¬¡ä¿®æ”¹åŠ ï¼‘å§
atomic_t i_count; Â  Â  Â  Â  Â Â Â Â Â Â
atomic_t i_dio_count;
atomic_t i_writecount;
const struct file_operations *i_fop; /* former ->i_op->default_file_ops */
struct file_lock *i_flock;
struct address_space i_data;
#ifdef CONFIG_QUOTA
struct dquot *i_dquot[MAXQUOTAS];
#endif
struct list_head i_devices;
union {
struct pipe_inode_info *i_pipe;
struct block_device *i_bdev;
struct cdev *i_cdev;
Â  Â  Â  Â Â  };

__u32 i_generation; Â  Â  Â  Â  Â //get_seconds();åº”è¯¥æ˜¯æ–‡ä»¶äº§ç”Ÿæ—¶é—´ï¼Œç›®å½•æœ€ä½Žä½æ¸…ï¼ï¼Œæ–‡ä»¶æœ€ä½Žä½è®¾ï¼‘

#ifdef CONFIG_FSNOTIFY
__u32 i_fsnotify_mask; /* all events this inode cares about */
struct hlist_head i_fsnotify_marks;
#endif

#ifdef CONFIG_IMA
atomic_t i_readcount; /* struct files open RO */
#endif
void *i_private; /* fs or device private pointer */
};

ç£ç›˜ä¸Šä¿å­˜çš„æ–‡ä»¶ç³»ç»Ÿæ•°æ®ç»“æž„

æ˜¯å¦åº”è¯¥å¢žåŠ ä¸€ä¸ªç®¡ç†ç©ºé—²ç°‡çš„é“¾è¡¨ï¼Œæ¯ä¸ªèŠ‚ç‚¹éƒ½åŒ…å«è¿žç»­ç°‡çš„ä¸ªæ•°ï¼Œå¹¶ä¸”æŒ‰ç…§ä¸€å®šçš„è§„å¾‹æŽ’åºã€‚ä¹Ÿè®¸å¯ä»¥ä½¿ç”¨ï½ˆï½ï½“ï½ˆ

ç³»ç»Ÿå¯ä»¥å®šä¹‰RESERVED_SECTORSä¿ç•™æ‰‡åŒºï¼Œæ¯”å¦‚æ˜¯ï¼“ï¼’ï¼Œæ€»æ‰‡åŒºä¸ºtotal,é‚£ä¹ˆï¼Œ
é€šè¿‡MBRä¸­çš„åˆ†åŒºè¡¨ä¿¡æ¯å¾—çŸ¥åˆ†åŒºçš„èµ·å§‹ä½ç½®ã€€é€šè¿‡åˆ†åŒºä¸­DBRå¾—çŸ¥DBRçš„ä¿ç•™æ‰‡åŒºæ•°ä»¥åŠFATè¡¨çš„å¤§å°ï¼ŒFATè¡¨çš„ä¸ªæ•°
FAT1=åˆ†åŒºèµ·å§‹æ‰‡åŒº+DBRä¿ç•™æ‰‡åŒºï¼ŒFAT2=åˆ†åŒºèµ·å§‹æ‰‡åŒº+DBRä¿ç•™æ‰‡åŒº+FAT1æ‰‡åŒºã€€æ•°æ®åŒºçš„ä½ç½®åœ¨FAT2çš„åŽé¢
æ ¹ç›®å½•=æ•°æ®åŒºçš„èµ·å§‹æ‰‡åŒº+ï¼ˆç°‡å¤§å°*2ï¼‰ï¼å’Œï¼‘ä¸¤ä¸ªç°‡è¢«é¢„ç•™ï¼Œè®¾ç½®0xffffffff,ã€€ï¼’å·ç°‡ç»™æ ¹ç›®å½•ï¼Œè®¾ç½®0x0fffffff8æ–‡ä»¶ç»“æŸæ ‡è®°ï¼Œæ–‡ä»¶å’Œç›®å½•å†…å®¹æ•°æ®ç”¨ç°‡æ¥ç®¡ç†ï¼Œå‰é¢çš„ä¿¡æ¯ç”¨æ‰‡åŒºå•ä½æ¥ç®¡ç†ã€‚
æ•°æ®åŒºçš„ç±»å®¹ä¸»è¦ç”±ä¸‰éƒ¨åˆ†ç»„æˆï¼šæ ¹ç›®å½•ï¼Œå­ç›®å½•å’Œæ–‡ä»¶å†…å®¹ã€‚åœ¨æ•°æ®åŒºä¸­æ˜¯ä»¥â€œç°‡â€ä¸ºå•ä½è¿›è¡Œå­˜å‚¨çš„ï¼Œ2å·ç°‡è¢«åˆ†é…ç»™æ ¹ç›®å½•ä½¿ç”¨
æ ¹ç›®å½•çš„å®šä½æ–¹å¼ä¸ºï¼šæ ¹ç›®å½•=åˆ†åŒºèµ·å§‹æ‰‡åŒº+DBRä¿ç•™æ‰‡åŒº+ï¼ˆFATè¡¨*2ï¼‰+ï¼ˆç°‡å¤§å°*2ï¼‰

fat32ç³»ç»Ÿçš„fatè¡¨ä¸­ï¼Œ0x0ffffff7ä»£è¡¨ä¸€ä¸ªåç°‡ï¼Œ0x0fffffffä»£è¡¨æ–‡ä»¶æœ€åŽä¸€ä¸ªclusterï¼ŒFAT[0]å¿…ç„¶æ˜¯0x0ffffff8ï¼Œ
struct fat_boot_sector { Â  Â  //æ€»é•¿åº¦64ä¸ªå­—èŠ‚
__u8 ignored[3]; /* Boot strap short or near jump */
__u8 system_id[8]; /* Name - can be used to special case partition manager volumes æˆ–è€…æ–‡ä»¶ç³»ç»Ÿå’Œç‰ˆæœ¬å·ç­‰*/
__u8 sector_size[2]; /* bytes per logical sector */ Â  Â  //11, ä¸€ä¸ªé€»è¾‘æ‰‡åŒº512byte
__u8 sec_per_clus; /* sectors/cluster */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  //13,ä¸€ä¸ª4kå¤§å°çš„clusteræœ‰8ä¸ªæ‰‡åŒº
__le16 reserved; /* reserved sectors */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â ã€€//14ã€€é¢„ç•™æ‰‡åŒºä¸ªæ•°ï¼Œä¹Ÿå°±æ˜¯ï½†ï½ï½”è¡¨ï¼‘å¼€å§‹æ‰‡åŒº
__u8 fats; /* number of FATs */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  //16, 2ä¸ªfatè¡¨
__u8 dir_entries[2]; /* root directory entries */ Â  Â  Â  Â  //17, Â 0, ä¸ç”¨
__u8 sectors[2]; /* number of sectors */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â //19, 0, ä¸ç”¨
__u8 media; /* media code */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Â //21,
Â  Â  Â  Â  Â  //0xf8è¡¨ç¤ºçš„æ˜¯non-removable media,0XF0è¡¨ç¤ºçš„æ˜¯removable mediaï¼Œè¯¥å€¼è¦å’ŒFAT[0]çš„ä½Žä½ç›¸åŒ
__le16 fat_length; /* sectors/FAT */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Â //22, 0, ä¸ç”¨ï¼Œç”¨ä¸‹é¢çš„fat32çš„length
__le16 secs_track; /* sectors per track */ Â  Â  Â  Â  Â  Â  Â  Â  Â Â //24,
__le16 heads; /* number of heads */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â //26,
__le32 hidden; /* hidden sectors (unused) */ Â  Â  Â  Â  Â Â //28,
Â  Â  Â  Â  Â  Â  Â  Â //å¯¹äºŽä¸å¸¦åˆ†åŒºçš„è®¾å¤‡ï¼Œæ­¤å€¼ä¸º0ï¼Œå¯¹äºŽå…¶ä»–çš„ï¼Œæ­¤å€¼çš„å«ä¹‰æ˜¯åœ¨partitionå‰çš„éšè—sectorçš„ä¸ªæ•°
__le32 total_sect; /* number of sectors (if sectors == 0) */ Â  Â  //32,è®¾å¤‡æ€»æ‰‡åŒºä¸ªæ•°
union {
struct { ... } fat16;
struct {
Â  Â  __le32 length; /* sectors/FAT */ Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â //36, ä¸€ä¸ªFATè¡¨å ç”¨çš„sectorçš„ä¸ªæ•°
Â  Â  __le16 flags; /* bit 8: fat mirroring, low 4: active fat */Â //40,
Â  Â  Â  Â  Â  //Bit7ä¸º0è¡¨ç¤ºçš„æ˜¯åœ¨è¿è¡Œçš„æ—¶å€™FATè¡¨1å’ŒFATè¡¨2æ˜¯äº’ä¸ºé•œåƒçš„ï¼Œbit7ä¸º1è¡¨ç¤ºåœ¨è¿è¡Œçš„æ—¶å€™åªæœ‰ä½Ž4ä½FATè¡¨æ˜¯æ´»åŠ¨çš„
Â  Â  __u8 version[2]; /* major, minor filesystem version */Â //42,
Â  Â  __le32 root_cluster; /* first cluster in root directory */Â //44,
Â  Â Â  Â Â Â Â Â //æ ¹ç›®å½•æ‰€åœ¨clusterçš„entryé€šå¸¸ä¸º2ï¼Œè¿™æ ·ä»Žæ•°æ®åŒºçš„cluster2(0å¼€å§‹)å¯ä»¥æ‰¾åˆ°æ ¹ç›®å½•
Â  Â  __le16 info_sector; /* filesystem info sector */ Â  Â  Â  Â  Â //48,è¡¨æ˜Žäº†fsinfoå ç”¨å“ªä¸ªæ‰‡åŒºå·,ã€€å¦‚æžœæ˜¯ï¼æ”¹ä¸ºï¼‘
Â  Â  __le16 backup_boot; /* backup boot sector */ Â  Â  Â  Â  Â //50,å¤‡ä»½çš„å¼•å¯¼æ‰‡åŒºï¼Œä¸€èˆ¬æ˜¯ï¼–
Â  Â  __le16 reserved2[6]; /* Unused *//* Extended BPB Fields for FAT32 */Â //52,
Â  Â  __u8 drive_number; /* Physical drive number */Â //64, å®žé™…å±žäºŽfat_boot_bsxå®šä¹‰
Â  Â  __u8 state; /* undocumented, but used for mount state. */Â //65,ã€€å®žé™…å±žäºŽfat_boot_bsxå®šä¹‰
Â  Â  } fat32;
Â  };
};
#define FAT32_BSX_OFFSET 64 /* offset of fat_boot_bsx in FAT32 */ Â //struct fat_boot_sector é•¿åº¦ä¸º64
#define MSDOS_ROOT_INO 1 Â  /* The root inode number */Â Â Â Â Â
#define MSDOS_FSINFO_INO 2 /* Used for managing the FSINFO block */

struct fat_boot_bsx {Â Â Â Â Â //åŸºæœ¬æ²¡ç”¨
__u8 drive; /* drive number */
__u8 reserved1;
__u8 signature; /* extended boot signature */
__u8 vol_id[4]; /* volume ID */ Â  Â  //å”¯ä¸€æœ‰ç”¨çš„ä¸€ä¸ªæˆå‘˜å˜é‡ï¼Œå¤åˆ¶ç»™struct msdos_sb_infoçš„vol_id
__u8 vol_label[11]; /* volume label, */
__u8 type[8]; /* file system type , fat32*/
};
struct fat_boot_fsinfo {ã€€//ä¸€èˆ¬åœ¨ç¬¬äºŒä¸ªæ‰‡åŒºï¼Œä½†ä¹Ÿå°±ä¿æœ‰ä¸¤ä¸ªæœ‰ç”¨çš„ä¿¡æ¯è€Œå·²ï¼Œç”¨äºŽç©ºé—²clusterçš„æŸ¥æ‰¾åˆ†é…
__le32 signature1; /* 0x41615252L */
__le32 reserved1[120]; /* Nothing as far as I can tell */
__le32 signature2; /* 0x61417272L */
__le32 free_clusters; /* Free cluster count. -1 if unknown */ Â  Â //488,ç©ºé—²clusterçš„ä¸ªæ•°ï¼Œ-1ä»£è¡¨å½“å‰æœªçŸ¥
__le32 next_cluster; /* Most recently allocated cluster */ Â  Â  //æœ€è¿‘è¢«åˆ†é…çš„cluster,ä¸‹æ¬¡åˆ†é…å¯ä»¥ä»Žæ­¤å¼€å§‹
__le32 reserved2[4]; //508,Â 0xAA550000
};

struct msdos_dir_entry { Â  Â  //æ–‡ä»¶æˆ–è€…ç›®å½•çš„åŸºæœ¬ä¿¡æ¯ï¼ŒçŸ­æ–‡ä»¶å
__u8 name[MSDOS_NAME];/* name and extension */ //11ä¸ªå­—èŠ‚é•¿åº¦çš„åå­—
__u8 attr; /* attribute bits */ Â  Â  Â  Â  Â  Â  Â  //12, æ–‡ä»¶å±žæ€§
__u8 lcase; /* Case for base and extension */
__u8 ctime_cs; /* Creation time, centiseconds (0-199) */ Â  //13,
__le16 ctime; /* Creation time */ Â  Â Â //14,
__le16 cdate; /* Creation date */ Â  Â Â //16,
__le16 adate; /* Last access date */ Â  Â Â //18,
__le16 starthi; /* High 16 bits of cluster in FAT32 */ Â  Â  //20, æ–‡ä»¶æ•°æ®å¼€å§‹clusterçš„entryçš„é«˜16ä½
__le16 time,date;Â /* time, date */
__le16 start;/* first cluster */ Â  Â Â //26, Â æ–‡ä»¶æ•°æ®å¼€å§‹clusterçš„entryçš„ä½Ž16ä½
__le32 size; /* file size (in bytes) */ Â  Â Â //20,ã€€æ–‡ä»¶å®žé™…å¤§å°
};
struct fat_slot_info {
loff_t i_pos; /* on-disk position of directory entry æ–‡ä»¶ç›®å½•é¡¹ç´¢å¼•*/
loff_t slot_off; /* offset for slot or de start */
int nr_slots; /* number of slots + 1(de) in filename */
struct msdos_dir_entry *de;
struct buffer_head *bh;
};
struct msdos_dir_slot {
__u8 id; /* sequence number for slot */
__u8 name0_4[10]; /* first 5 characters in name */
__u8 attr; /* attribute byte */
__u8 reserved; /* always 0 */
__u8 alias_checksum; /* checksum for 8.3 alias */
__u8 name5_10[12]; /* 6 more characters in name */
__le16 start; /* starting cluster number, 0 in long slots */
__u8 name11_12[4]; /* last 2 characters in name */
};

struct msdos_sb_info {//è¶…çº§å—fat32ç§æœ‰ä¿¡æ¯ï¼Œè¶…çº§å—s_fs_infoæŒ‡é’ˆæŒ‡å‘å½“å‰ç»“æž„ä½“,ä»Žç£ç›˜å‰ä¸¤ä¸ªbootæ‰‡åŒºé‡‡é›†
Â Â Â Â Â unsigned short sec_per_clus; /* sectors/cluster Â  Â  Â ï¼”ï½‹ï¼ï¼•ï¼‘ï¼’ï¼ï¼˜*/
Â Â Â Â Â unsigned short cluster_bits; /* log2(cluster_size) ï¼Œï¼‘ï¼’ï½‚ï½‰ï½”ï½“*/
Â Â Â Â Â unsigned int cluster_size; /* cluster size ï¼Œ Â  Â  Â  Â  Â  Â  Â  ï¼”ï½‹*/
Â Â Â Â Â unsigned char fats;/* number of FATs, Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  ä¸¤ä¸ªfatè¡¨*/
Â Â Â Â Â unsigned char fat_bits; /* fat32, Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  32 bits */
Â Â Â Â Â unsigned short fat_start; Â  Â  Â  Â  Â  Â  Â  //fatè¡¨å¼€å§‹ä½ç½®çš„æ‰‡åŒºå—å·
Â Â Â Â Â unsigned long fat_length; /* FAT start & length (sec.) */ Â  Â  //å•ä¸ªï½†ï½ï½”è¡¨å çš„æ‰‡åŒºå—æ•°,åº”è¯¥æ˜¯ç°‡å¯¹é½çš„å§
Â Â Â Â Â unsigned long dir_start; Â  Â  Â  Â  Â  Â  Â  ï¼ï¼ï½†ï½ï½”è¡¨åŽæ ¹ç›®å½•å¼€å§‹ä½ç½®ï¼Œé•¿è‹¥å¹²ä¸ªç°‡ï¼ŒåŽé¢å°±æ˜¯data_start
Â Â Â Â Â unsigned short dir_entries; /* root dir start & entries ã€€æœ€å¤§æ ¹ç›®å½•ä¸ªæ•°*/
Â Â Â Â Â unsigned long data_start; /* first data sector */ Â  Â  Â  Â  Â ï¼ï¼æ–‡ä»¶æ•°æ®åŒºå¼€å§‹ä½ç½®
Â Â Â Â Â unsigned long max_cluster; /* maximum cluster number */
Â Â Â Â Â unsigned long root_cluster; /* first cluster of the root directory ï¼Œä¸€èˆ¬ä¸ºï¼’ï¼Œæ ¹ç›®å½•çš„é¦–ä¸ªç°‡å·*/
Â Â Â Â Â unsigned long fsinfo_sector; /* sector number of FAT32 fsinfo ï¼Œã€€ä¸€èˆ¬ä¸ºï¼‘*/
Â Â Â Â Â struct mutex fat_lock;
Â Â Â Â Â struct mutex nfs_build_inode_lock;
Â Â Â Â Â struct mutex s_lock;
Â Â Â Â Â unsigned int prev_free; /* previously allocated cluster number ,åˆšè¢«åˆ†é…å‡ºåŽ»çš„ç©ºé—²ç°‡å·*/
Â Â Â Â Â unsigned int free_clusters; /* -1 if undefined ã€€ç©ºé—²clusterçš„ä¸ªæ•°*/
Â Â Â Â Â unsigned int free_clus_valid; /* is free_clusters valid? ï¼Œfree_clustersæˆå‘˜çš„å€¼æ˜¯å¦æœ‰æ•ˆ*/
Â Â Â Â Â struct fat_mount_options options;
Â Â Â Â Â struct nls_table *nls_disk; /* Codepage used on disk */
Â Â Â Â Â struct nls_table *nls_io; /* Charset used for input and display */
Â Â Â Â Â const void *dir_ops; /* Opaque; default directory operations */
Â Â Â Â Â int dir_per_block; /* dir entries per block Â  Â  Â  Â  Â ï¼•ï¼‘ï¼’ï¼ï¼“ï¼’ï¼ï¼‘ï¼– */
Â Â Â Â Â int dir_per_block_bits; /* log2(dir_per_block) Â  Â  Â  Â  Â ï¼”ä¸ªï½‚ï½‰ï½”ï½“ */
Â Â Â Â Â unsigned long vol_id; /* volume ID */ Â  Â  //ä»ŽåŽŸå§‹æ‰‡åŒºä¸­struct fat_boot_bsxçš„vol_id[4]æ•°ç»„å¾—åˆ°

Â Â Â Â Â int fatent_shift; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Â  //2, ä»£è¡¨ï¼”ä¸ªbytes
Â Â Â Â Â struct fatent_operations *fatent_ops; Â  Â  Â Â Â Â Â //fatè¡¨ä¸­cluster entryçš„æ“ä½œå‡½æ•°
Â Â Â Â Â struct inode *fat_inode; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Â //åº”è¯¥æ˜¯ï½†ï½ï½”è¡¨å¯¹åº”çš„ï½‰ï½Žï½ï½„ï½…
Â Â Â Â Â struct inode *fsinfo_inode; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â  Â Â //æ‰‡åŒºï¼‘çš„fsinfoå¯¹åº”çš„ï¼©ï½Žï½ï½„ï½…
Â Â Â Â Â struct ratelimit_state ratelimit; Â  Â  Â  Â  Â  Â  Â  Â  Â  Â //æˆå‘˜å˜é‡interval åˆå§‹åŒ–ä¸º ï¼•,Â burst åˆå§‹åŒ–ä¸º 10ï¼Œratelimit_state_init()å‡½æ•°ä¸­

Â Â Â Â Â spinlock_t inode_hash_lock; Â spinlock_t dir_hash_lock;
Â Â Â Â Â struct hlist_head inode_hashtable[FAT_HASH_SIZE]; Â  Â Â //
Â Â Â Â Â struct hlist_head dir_hashtable[FAT_HASH_SIZE]; Â  Â  Â  Â  Â //

Â Â Â Â Â unsigned int dirty; /* fs state before mount */ Â  Â  Â  Â  Â  Â  Â Â //
};

struct msdos_inode_info {
spinlock_t cache_lru_lock;
struct list_head cache_lru;
int nr_caches;
/* for avoiding the race between fat_free() and fat_get_cluster() */
unsigned int cache_valid_id;

/* NOTE: mmu_private is 64bits, so must hold ->i_mutex to access */
loff_t mmu_private; /* physically allocated size */

int i_start; /* first cluster or 0 */
int i_logstart; /* logical first cluster */
int i_attrs; /* unused attribute bits */
loff_t i_pos; /* on-disk position of directory entry or 0 æ–‡ä»¶ç›®å½•é¡¹ç´¢å¼•*/
struct hlist_node i_fat_hash; /* hash by i_location */
struct hlist_node i_dir_hash; /* hash by i_logstart */
struct rw_semaphore truncate_lock; /* protect bmap against truncate */
struct inode vfs_inode;
};

struct dentry {
Â Â  Â /* RCU lookup touched fields */
Â Â  Â unsigned int d_flags;Â Â  Â Â Â  Â /* protected by d_lockÂ  ç›®å½•é¡¹æ ‡å¿—*/
Â Â  Â seqcount_t d_seq;Â Â  Â Â Â  Â /* per dentry seqlock */
Â Â  Â struct hlist_bl_node d_hash;Â Â  Â /* lookup hash list æ•£åˆ—è¡¨è¡¨é¡¹çš„æŒ‡é’ˆ*/
Â Â  Â struct dentry *d_parent;Â Â  Â /* parent directory çˆ¶ç›®å½•çš„ç›®å½•é¡¹å¯¹è±¡*/
Â Â  Â struct qstr d_name; Â Â Â Â  Â Â Â  //æ–‡ä»¶å
Â Â  Â struct inode *d_inode;Â Â  Â Â Â  Â /* Where the name belongs to - NULL isã€€* negative * ä¸Žæ–‡ä»¶åå…³è”çš„ç´¢å¼•èŠ‚ç‚¹/
Â Â  Â unsigned char d_iname[DNAME_INLINE_LEN];Â Â  Â /* small names å­˜æ”¾çŸ­æ–‡ä»¶å*/

Â Â  Â /* Ref lookup also touches following */
Â Â  Â unsigned int d_count;Â Â  Â Â Â  Â /* protected by d_lock ç›®å½•é¡¹å¯¹è±¡ä½¿ç”¨è®¡æ•°å™¨*/
Â Â  Â spinlock_t d_lock;Â Â  Â Â Â  Â /* per dentry lock */
Â Â  Â const struct dentry_operations *d_op;
Â Â  Â struct super_block *d_sb;Â Â  Â /* The root of the dentry tree */
Â Â  Â unsigned long d_time;Â Â  Â Â Â  Â /* used by d_revalidate */
Â Â  Â void *d_fsdata;Â Â  Â Â Â  Â Â Â  Â /* fs-specific data */

Â Â  Â struct list_head d_lru;Â Â  Â Â Â  Â /* LRU list æœ€è¿‘æœ€å°‘ä½¿ç”¨é“¾è¡¨çš„æŒ‡é’ˆ, d_count<=0çš„dentryåº”è¯¥åœ¨æ­¤é“¾è¡¨ä¸­å§*/
Â Â  Â /*Â Â  Â  * d_child and d_rcu can share memoryÂ Â  Â  */
Â Â  Â union {
Â Â  Â Â Â  Â struct list_head d_child;Â Â  Â /* child of parent list çˆ¶ç›®å½•ä¸­ç›®å½•é¡¹å¯¹è±¡çš„é“¾è¡¨çš„æŒ‡é’ˆï¼Œå…„å¼Ÿä»¬*/
Â Â  Â  Â Â  Â struct rcu_head d_rcu;
Â Â  Â } d_u;
Â Â  Â struct list_head d_subdirs;Â Â  Â /* our children å¯¹ç›®å½•è€Œè¨€ï¼Œè¡¨ç¤ºå­ç›®å½•ç›®å½•é¡¹å¯¹è±¡çš„é“¾è¡¨*/
Â Â  Â struct hlist_node d_alias;Â Â  Â /* inode alias list ç›¸å…³ç´¢å¼•èŠ‚ç‚¹ï¼ˆåˆ«åï¼‰çš„é“¾è¡¨*/
};

