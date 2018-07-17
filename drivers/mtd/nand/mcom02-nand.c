/*
 * Copyright (C) 2014 - 2015 Xilinx, Inc.
 * Copyright (C) 2015 ELVEES NeoTek, CJSC
 * Copyright 2018 RnD Center "ELVEES", JSC
 * Based on the Linux version of Arasan NFC driver.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <nand.h>
#include <linux/errno.h>

/* Register offsets */
#define PKT_OFST			0x00
#define MEM_ADDR1_OFST			0x04
#define MEM_ADDR2_OFST			0x08
#define CMD_OFST			0x0C
#define PROG_OFST			0x10
#define INTR_STS_EN_OFST		0x14
#define INTR_SIG_EN_OFST		0x18
#define INTR_STS_OFST			0x1C
#define ID1_OFST			0x20
#define ID2_OFST			0x24
#define FLASH_STS_OFST			0x28
#define DATA_PORT_OFST			0x30
#define ECC_OFST			0x34
#define ECC_ERR_CNT_OFST		0x38
#define ECC_SPR_CMD_OFST		0x3C
#define ECC_ERR_CNT_1BIT_OFST		0x40
#define ECC_ERR_CNT_2BIT_OFST		0x44

#define PKT_CNT_SHIFT			12

#define ECC_ENABLE			BIT(31)
#define PAGE_SIZE_MASK			GENMASK(25, 23)
#define PAGE_SIZE_SHIFT			23
#define PAGE_SIZE_512			0
#define PAGE_SIZE_2K			1
#define PAGE_SIZE_4K			2
#define PAGE_SIZE_8K			3
#define CMD2_SHIFT			8
#define ADDR_CYCLES_SHIFT		28

#define XFER_COMPLETE			BIT(2)
#define READ_READY			BIT(1)
#define WRITE_READY			BIT(0)
#define MBIT_ERROR			BIT(3)
#define ERR_INTRPT			BIT(4)

#define PROG_PGRD			BIT(0)
#define PROG_ERASE			BIT(2)
#define PROG_STATUS			BIT(3)
#define PROG_PGPROG			BIT(4)
#define PROG_RDID			BIT(6)
#define PROG_RDPARAM			BIT(7)
#define PROG_RST			BIT(8)

#define ONFI_STATUS_FAIL		BIT(0)
#define ONFI_STATUS_READY		BIT(6)

#define PG_ADDR_SHIFT			16
#define BCH_MODE_SHIFT			25
#define BCH_EN_SHIFT			25
#define ECC_SIZE_SHIFT			16

#define MEM_ADDR_MASK			GENMASK(7, 0)
#define BCH_MODE_MASK			GENMASK(27, 25)

#define CS_MASK				GENMASK(31, 30)
#define CS_SHIFT			30

#define PAGE_ERR_CNT_MASK		GENMASK(16, 8)
#define PAGE_ERR_CNT_SHIFT              8
#define PKT_ERR_CNT_MASK		GENMASK(7, 0)

#define ONFI_ID_ADDR			0x20
#define ONFI_ID_LEN			4
#define MAF_ID_LEN			5
#define DMA_BUFSIZE			SZ_64K
#define TEMP_BUF_SIZE			512
#define SPARE_ADDR_CYCLES		BIT(29)

#define ARASAN_NAND_POLL_TIMEOUT	1000000
#define STATUS_TIMEOUT			2000

DECLARE_GLOBAL_DATA_PTR;

struct mcom02_nand_priv {
	struct udevice *dev;
	struct nand_chip nand;

	void __iomem *regs;

	u8 buf[TEMP_BUF_SIZE];

	bool bch;
	bool err;
	bool iswriteoob;
	u16 raddr_cycles;
	u16 caddr_cycles;
	u32 page;
	u32 bufshift;
	u32 rdintrmask;
	u32 pktsize;
	u32 ecc_regval;

	int curr_cmd;
	struct nand_ecclayout ecclayout;
};

struct mcom02_nand_ecc_matrix {
	u32 pagesize;
	u32 codeword_size;
	u8 eccbits;
	u8 bch;
	u16 eccsize;
};

static const struct mcom02_nand_ecc_matrix ecc_matrix[] = {
	{512,	512,	1,	0,	0x3},
	{512,	512,	4,	1,	0x7},
	{512,	512,	8,	1,	0xD},
	/* 2K byte page */
	{2048,	512,	1,	0,	0xC},
	{2048,	512,	4,	1,	0x1A},
	{2048,	512,	8,	1,	0x34},
	{2048,	512,	12,	1,	0x4E},
	{2048,	512,	16,	1,	0x68},
	{2048,	1024,	24,	1,	0x54},
	/* 4K byte page */
	{4096,	512,	1,	0,	0x18},
	{4096,	512,	8,	1,	0x68},
	{4096,	512,	16,	1,	0xD0},
	/* 8K byte page */
	{8192,	512,	1,	0,	0x30},
	{8192,	512,	8,	1,	0xD0},
	{8192,	512,	12,	1,	0x138},
	{8192,	512,	16,	1,	0x1A0},
};

static u8 mcom02_nand_page(u32 pagesize)
{
	switch (pagesize) {
	case 512:
		return PAGE_SIZE_512;
	case 2048:
		return PAGE_SIZE_2K;
	case 4096:
		return PAGE_SIZE_4K;
	case 8192:
		return PAGE_SIZE_8K;
	default:
		printf("Unsupported page size: %#x\n", pagesize);
		break;
	}

	return 0;
}

static void mcom02_nand_prepare_cmd(struct mcom02_nand_priv *priv, u8 cmd1,
				    u8 cmd2, u32 pagesize, u8 addrcycles)
{
	u32 regval;

	regval = cmd1 | (cmd2 << CMD2_SHIFT);
	if (addrcycles)
		regval |= addrcycles << ADDR_CYCLES_SHIFT;
	if (pagesize)
		regval |= mcom02_nand_page(pagesize) << PAGE_SIZE_SHIFT;
	writel(regval, priv->regs + CMD_OFST);
}

static void mcom02_nand_setpagecoladdr(struct mcom02_nand_priv *priv,
				       u32 page, u16 col)
{
	u32 val;

	writel(col | (page << PG_ADDR_SHIFT), priv->regs + MEM_ADDR1_OFST);

	val = readl(priv->regs + MEM_ADDR2_OFST);
	val = (val & ~MEM_ADDR_MASK) |
	      ((page >> PG_ADDR_SHIFT) & MEM_ADDR_MASK);
	writel(val, priv->regs + MEM_ADDR2_OFST);
}

static inline void mcom02_nand_setpktszcnt(struct mcom02_nand_priv *priv,
					   u32 pktsize, u32 pktcount)
{
	writel(pktsize | (pktcount << PKT_CNT_SHIFT), priv->regs + PKT_OFST);
}

static inline void mcom02_nand_set_irq_masks(struct mcom02_nand_priv *priv,
					     u32 val)
{
	writel(val, priv->regs + INTR_STS_EN_OFST);
}

static void mcom02_nand_wait_for_event(struct mcom02_nand_priv *priv,
				       u32 event)
{
	u32 timeout = ARASAN_NAND_POLL_TIMEOUT;

	while (!(readl(priv->regs + INTR_STS_OFST) & event) && timeout) {
		udelay(1);
		timeout--;
	}

	if (!timeout)
		printf("Event waiting timeout, %d\n", event);

	writel(event, priv->regs + INTR_STS_OFST);
}

static void mcom02_nand_readfifo(struct mcom02_nand_priv *priv, u32 prog,
				 u32 size)
{
	u32 i, *bufptr = (u32 *)&priv->buf[0];

	mcom02_nand_set_irq_masks(priv, READ_READY);

	writel(prog, priv->regs + PROG_OFST);
	mcom02_nand_wait_for_event(priv, READ_READY);

	mcom02_nand_set_irq_masks(priv, XFER_COMPLETE);

	for (i = 0; i < size / 4; i++)
		bufptr[i] = readl(priv->regs + DATA_PORT_OFST);

	mcom02_nand_wait_for_event(priv, XFER_COMPLETE);
}

static void mcom02_nand_cmdfunc(struct mtd_info *mtd, unsigned int cmd,
				int column, int page_addr)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	bool wait = false, read = false, read_id = false;
	u32 addrcycles, prog;
	u32 *bufptr = (u32 *)&priv->buf[0];

	priv->bufshift = 0;
	priv->curr_cmd = cmd;

	if (page_addr == -1)
		page_addr = 0;
	if (column == -1)
		column = 0;

	switch (cmd) {
	case NAND_CMD_RESET:
		mcom02_nand_prepare_cmd(priv, cmd, 0, 0, 0);
		prog = PROG_RST;
		wait = true;
		break;
	case NAND_CMD_SEQIN:
		addrcycles = priv->raddr_cycles + priv->caddr_cycles;
		priv->page = page_addr;
		mcom02_nand_prepare_cmd(priv, cmd, NAND_CMD_PAGEPROG,
					mtd->writesize, addrcycles);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		break;
	case NAND_CMD_READOOB:
		column += mtd->writesize;
	case NAND_CMD_READ0:
	case NAND_CMD_READ1:
		addrcycles = priv->raddr_cycles + priv->caddr_cycles;
		mcom02_nand_prepare_cmd(priv, NAND_CMD_READ0,
					NAND_CMD_READSTART,
					mtd->writesize, addrcycles);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		break;
	case NAND_CMD_RNDOUT:
		mcom02_nand_prepare_cmd(priv, cmd, NAND_CMD_RNDOUTSTART,
					mtd->writesize, 2);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		priv->rdintrmask = READ_READY;
		break;
	case NAND_CMD_PARAM:
		mcom02_nand_prepare_cmd(priv, cmd, 0, 0, 1);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		mcom02_nand_setpktszcnt(priv, sizeof(struct nand_onfi_params),
					1);
		mcom02_nand_readfifo(priv, PROG_RDPARAM,
				     sizeof(struct nand_onfi_params));
		break;
	case NAND_CMD_READID:
		mcom02_nand_prepare_cmd(priv, cmd, 0, 0, 1);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		if (column == ONFI_ID_ADDR)
			mcom02_nand_setpktszcnt(priv, ONFI_ID_LEN, 1);
		else
			mcom02_nand_setpktszcnt(priv, MAF_ID_LEN, 1);
		prog = PROG_RDID;
		wait = true;
		read_id = true;
		break;
	case NAND_CMD_ERASE1:
		addrcycles = priv->raddr_cycles;
		mcom02_nand_prepare_cmd(priv, cmd, NAND_CMD_ERASE2,
					0, addrcycles);
		column = page_addr & 0xffff;
		page_addr = (page_addr >> PG_ADDR_SHIFT) & 0xffff;
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		prog = PROG_ERASE;
		wait = true;
		break;
	case NAND_CMD_STATUS:
		mcom02_nand_prepare_cmd(priv, cmd, 0, 0, 0);
		mcom02_nand_setpktszcnt(priv, 1, 1);
		mcom02_nand_setpagecoladdr(priv, page_addr, column);
		prog = PROG_STATUS;
		wait = true;
		read = true;
		break;
	default:
		return;
	}

	if (wait) {
		mcom02_nand_set_irq_masks(priv, XFER_COMPLETE);
		writel(prog, priv->regs + PROG_OFST);
		mcom02_nand_wait_for_event(priv, XFER_COMPLETE);
	}

	if (read)
		bufptr[0] = readl(priv->regs + FLASH_STS_OFST);
	if (read_id) {
		bufptr[0] = readl(priv->regs + ID1_OFST);
		bufptr[1] = readl(priv->regs + ID2_OFST);
		if (column == ONFI_ID_ADDR)
			bufptr[0] = ((bufptr[0] >> 8) | (bufptr[1] << 24));
	}
}

static void mcom02_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u32 val;

	if (chip == -1)
		return;

	val = readl(priv->regs + MEM_ADDR2_OFST);
	val = (val & ~(CS_MASK)) | (chip << CS_SHIFT);
	writel(val, priv->regs + MEM_ADDR2_OFST);
}

static u8 mcom02_nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);

	return priv->buf[priv->bufshift++];
}

static void mcom02_nand_read_buf(struct mtd_info *mtd, u8 *buf, int size)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u32 i, pktcount, buf_rd_cnt = 0, pktsize;
	u32 *bufptr = (u32 *)buf;

	priv->rdintrmask |= READ_READY;

	if (priv->curr_cmd == NAND_CMD_READ0) {
		pktsize = priv->pktsize;
		if (mtd->writesize % pktsize)
			pktcount = mtd->writesize / pktsize + 1;
		else
			pktcount = mtd->writesize / pktsize;
	} else {
		pktsize = size;
		pktcount = 1;
	}

	mcom02_nand_setpktszcnt(priv, pktsize, pktcount);

	mcom02_nand_set_irq_masks(priv, priv->rdintrmask);
	writel(PROG_PGRD, priv->regs + PROG_OFST);

	while (buf_rd_cnt < pktcount) {
		mcom02_nand_wait_for_event(priv, READ_READY);
		buf_rd_cnt++;

		if (buf_rd_cnt == pktcount)
			mcom02_nand_set_irq_masks(priv, XFER_COMPLETE);

		for (i = 0; i < pktsize / 4; i++)
			bufptr[i] = readl(priv->regs + DATA_PORT_OFST);

		bufptr += (pktsize / 4);

		if (buf_rd_cnt < pktcount)
			mcom02_nand_set_irq_masks(priv, priv->rdintrmask);
	}

	mcom02_nand_wait_for_event(priv, XFER_COMPLETE);
	priv->rdintrmask = 0;
}

static inline void mcom02_nand_set_eccsparecmd(struct mcom02_nand_priv *priv,
					       u8 cmd1, u8 cmd2)
{
	writel(cmd1 | (cmd2 << CMD2_SHIFT) |
	      (priv->caddr_cycles << ADDR_CYCLES_SHIFT) | SPARE_ADDR_CYCLES,
	       priv->regs + ECC_SPR_CMD_OFST);
}

static int mcom02_nand_read_page_hwecc(struct mtd_info *mtd,
				       struct nand_chip *nand, u8 *buf,
				       int oob_required, int page)
{
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u32 val;

	mcom02_nand_set_eccsparecmd(priv, NAND_CMD_RNDOUT,
				    NAND_CMD_RNDOUTSTART);
	writel(priv->ecc_regval, priv->regs + ECC_OFST);

	val = readl(priv->regs + CMD_OFST);
	val = val | ECC_ENABLE;
	writel(val, priv->regs + CMD_OFST);

	if (!priv->bch)
		priv->rdintrmask = MBIT_ERROR;

	nand->read_buf(mtd, buf, mtd->writesize);

	val = readl(priv->regs + ECC_ERR_CNT_OFST);
	if (priv->bch) {
		mtd->ecc_stats.corrected +=
			(val & PAGE_ERR_CNT_MASK) >> PAGE_ERR_CNT_SHIFT;
	} else {
		val = readl(priv->regs + ECC_ERR_CNT_1BIT_OFST);
		mtd->ecc_stats.corrected += val;
		val = readl(priv->regs + ECC_ERR_CNT_2BIT_OFST);
		mtd->ecc_stats.failed += val;
		/* clear ecc error count register 1Bit, 2Bit */
		writel(0x0, priv->regs + ECC_ERR_CNT_1BIT_OFST);
		writel(0x0, priv->regs + ECC_ERR_CNT_2BIT_OFST);
	}
	priv->err = false;

	if (oob_required)
		nand->ecc.read_oob(mtd, nand, page);

	return 0;
}

static int mcom02_nand_device_ready(struct mtd_info *mtd,
				    struct nand_chip *nand)
{
	u8 status;
	u32 timeout = STATUS_TIMEOUT;

	while (timeout--) {
		nand->cmdfunc(mtd, NAND_CMD_STATUS, 0, 0);
		status = nand->read_byte(mtd);

		if (status & ONFI_STATUS_READY) {
			if (status & ONFI_STATUS_FAIL)
				return NAND_STATUS_FAIL;

			return 0;
		}
	}

	printf("Device ready timedout\n");

	return -ETIMEDOUT;
}

static void mcom02_nand_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u32 buf_wr_cnt = 0, pktcount = 1, i, pktsize;
	u32 *bufptr = (u32 *)buf;

	if (priv->iswriteoob) {
		pktsize = len;
		pktcount = 1;
	} else {
		pktsize = priv->pktsize;
		pktcount = mtd->writesize / pktsize;
	}

	mcom02_nand_setpktszcnt(priv, pktsize, pktcount);

	mcom02_nand_set_irq_masks(priv, WRITE_READY);
	writel(PROG_PGPROG, priv->regs + PROG_OFST);

	while (buf_wr_cnt < pktcount) {
		mcom02_nand_wait_for_event(priv, WRITE_READY);

		buf_wr_cnt++;
		if (buf_wr_cnt == pktcount)
			mcom02_nand_set_irq_masks(priv, XFER_COMPLETE);

		for (i = 0; i < (pktsize / 4); i++)
			writel(bufptr[i], priv->regs + DATA_PORT_OFST);

		bufptr += (pktsize / 4);

		if (buf_wr_cnt < pktcount)
			mcom02_nand_set_irq_masks(priv, WRITE_READY);
	}

	mcom02_nand_wait_for_event(priv, XFER_COMPLETE);
}

static int mcom02_nand_write_page_hwecc(struct mtd_info *mtd,
					struct nand_chip *nand,
					const u8 *buf,
					int oob_required, int page)
{
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u8 *ecc_calc = nand->buffers->ecccalc;
	u32 *eccpos = nand->ecc.layout->eccpos;
	u32 val, i;

	mcom02_nand_set_eccsparecmd(priv, NAND_CMD_RNDIN, 0);
	writel(priv->ecc_regval, priv->regs + ECC_OFST);

	val = readl(priv->regs + CMD_OFST);
	val = val | ECC_ENABLE;
	writel(val, priv->regs + CMD_OFST);

	nand->write_buf(mtd, buf, mtd->writesize);

	if (oob_required) {
		mcom02_nand_device_ready(mtd, nand);
		nand->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		nand->read_buf(mtd, ecc_calc, mtd->oobsize);
		for (i = 0; i < nand->ecc.total; i++)
			nand->oob_poi[eccpos[i]] = ecc_calc[eccpos[i]];
		nand->ecc.write_oob(mtd, nand, page);
	}

	return 0;
}

static int mcom02_nand_read_oob(struct mtd_info *mtd, struct nand_chip *nand,
				int page)
{
	nand->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	nand->read_buf(mtd, nand->oob_poi, mtd->oobsize);

	return 0;
}

static int mcom02_nand_write_oob(struct mtd_info *mtd, struct nand_chip *nand,
				 int page)
{
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);

	priv->iswriteoob = true;
	nand->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
	nand->write_buf(mtd, nand->oob_poi, mtd->oobsize);
	priv->iswriteoob = false;

	return 0;
}

static int mcom02_nand_ecc_init(struct mtd_info *mtd)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct mcom02_nand_priv *priv = nand_get_controller_data(nand);
	u32 oob_index, i, regval, eccaddr, bchmode = 0;
	int found = -1;

	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.read_page = mcom02_nand_read_page_hwecc;
	nand->ecc.write_page = mcom02_nand_write_page_hwecc;
	nand->ecc.write_oob = mcom02_nand_write_oob;
	nand->ecc.read_oob = mcom02_nand_read_oob;

	for (i = 0; i < ARRAY_SIZE(ecc_matrix); i++) {
		if (ecc_matrix[i].pagesize == mtd->writesize) {
			if (ecc_matrix[i].eccbits >=
			    nand->ecc_strength_ds) {
				found = i;
				break;
			}
			found = i;
		}
	}

	if (found < 0) {
		printf("ECC scheme not supported\n");
		return 1;
	}

	if (ecc_matrix[found].bch) {
		switch (ecc_matrix[found].eccbits) {
		case 12:
			bchmode = 0x1;
			break;
		case 8:
			bchmode = 0x2;
			break;
		case 4:
			bchmode = 0x3;
			break;
		case 24:
			bchmode = 0x4;
			break;
		default:
			bchmode = 0x0;
		}
	}

	nand->ecc.strength = ecc_matrix[found].eccbits;
	nand->ecc.size = ecc_matrix[found].codeword_size;

	nand->ecc.steps = ecc_matrix[found].pagesize /
			       ecc_matrix[found].codeword_size;

	nand->ecc.bytes = ecc_matrix[found].eccsize /
			       nand->ecc.steps;

	priv->ecclayout.eccbytes = ecc_matrix[found].eccsize;
	priv->bch = ecc_matrix[found].bch;

	if (mtd->oobsize < ecc_matrix[found].eccsize + 2) {
		printf("OOB too small for ECC scheme\n");
		return 1;
	}
	oob_index = mtd->oobsize - priv->ecclayout.eccbytes;
	eccaddr = mtd->writesize + oob_index;

	for (i = 0; i < nand->ecc.size; i++)
		priv->ecclayout.eccpos[i] = oob_index + i;

	priv->ecclayout.oobfree->offset = 2;
	priv->ecclayout.oobfree->length = oob_index -
		priv->ecclayout.oobfree->offset;

	nand->ecc.layout = &priv->ecclayout;

	regval = eccaddr |
		(ecc_matrix[found].eccsize << ECC_SIZE_SHIFT) |
		(ecc_matrix[found].bch << BCH_EN_SHIFT);
	priv->ecc_regval = regval;
	writel(regval, priv->regs + ECC_OFST);

	regval = readl(priv->regs + MEM_ADDR2_OFST);
	regval = (regval & ~(BCH_MODE_MASK)) | (bchmode << BCH_MODE_SHIFT);
	writel(regval, priv->regs + MEM_ADDR2_OFST);

	if (nand->ecc.size >= 1024)
		priv->pktsize = 1024;
	else
		priv->pktsize = 512;

	return 0;
}

static int mcom02_nand_probe(struct udevice *dev)
{
	struct mcom02_nand_priv *priv = dev_get_priv(dev);
	struct nand_chip *nand = &priv->nand;
	struct mtd_info *mtd;
	int ret;

	priv->regs = (void *)devfdt_get_addr(dev);
	nand_set_controller_data(nand, priv);

	/* Set the driver entry points for MTD */
	nand->cmdfunc = mcom02_nand_cmdfunc;
	nand->select_chip = mcom02_nand_select_chip;
	nand->read_byte = mcom02_nand_read_byte;
	nand->waitfunc = mcom02_nand_device_ready;
	nand->chip_delay = 30;

	/* Buffer read/write routines */
	nand->read_buf = mcom02_nand_read_buf;
	nand->write_buf = mcom02_nand_write_buf;
	nand->options = NAND_BUSWIDTH_AUTO
			| NAND_NO_SUBPAGE_WRITE
			| NAND_USE_BOUNCE_BUFFER;
	nand->bbt_options = NAND_BBT_USE_FLASH;

	priv->rdintrmask = 0;

	mtd = nand_to_mtd(nand);
	ret = nand_scan_ident(mtd, 1, NULL);
	if (ret)
		return ret;

	if (mtd->writesize > SZ_8K) {
		printf("Page size too big for controller\n");
		return -EINVAL;
	}

	if (!nand->onfi_params.addr_cycles) {
		/* Good estimate in case ONFI ident doesn't work */
		priv->raddr_cycles = 3;
		priv->caddr_cycles = 2;
	} else {
		priv->raddr_cycles = nand->onfi_params.addr_cycles & 0xF;
		priv->caddr_cycles =
			(nand->onfi_params.addr_cycles >> 4) & 0xF;
	}

	if (mcom02_nand_ecc_init(mtd))
		return -ENXIO;

	ret = nand_scan_tail(mtd);
	if (ret)
		return ret;

	ret = nand_register(0, mtd);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id mcom02_nand_dt_ids[] = {
	{ .compatible = "arasan,nfc-v2p99" },
	{ },
};

U_BOOT_DRIVER(mcom02_nand) = {
	.name = "mcom02-nand",
	.id = UCLASS_MTD,
	.of_match = mcom02_nand_dt_ids,
	.probe = mcom02_nand_probe,
	.priv_auto_alloc_size = sizeof(struct mcom02_nand_priv),
};

void board_nand_init(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MTD,
					  DM_GET_DRIVER(mcom02_nand), &dev);
	if (ret && ret != -ENODEV)
		printf("Failed to initialize %s, error %d\n", dev->name, ret);
}
